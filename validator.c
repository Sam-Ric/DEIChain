/*
  DEIChain - Validator Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>

#include "utils.h"
#include "validator.h"
#include "pow.h"

#define BUF_SIZE 200

extern int tx_per_block;
extern int tx_pool_size;
extern int blockchain_blocks;
extern TxPoolNode *tx_pool;
extern TxBlock *blocks;

extern sem_t *ledger_mutex;
extern sem_t *tx_pool_mutex;
extern sem_t *tx_pool_empty;
extern sem_t *pipe_mutex;

void validator(int id) {
  // Process initialization
  char msg[BUF_SIZE];
  sprintf(msg, "[Validator %d] Process initialized (PID -> %d | parent PID -> %d)", id, getpid(), getppid());
  log_message(msg, 'r', DEBUG);

  int fd = open(PIPE_NAME, O_RDONLY);
  if (fd < 0) {
    sprintf(msg, "[Validator %d] Error opening the named pipe", id);
    log_message(msg, 'w', 1);
    return;
  }

  sprintf(msg, "[Validator %d] Successfully opened the named pipe", id);
  log_message(msg, 'r', DEBUG);

  PipeMsg recv;

  while (1) {
    // Read a block from the named pipe (blocking state while waiting)
    int bytes = read(fd, &recv, sizeof(PipeMsg));
    if (bytes < 0) {
      sprintf(msg, "[Validator %d] Error reading from the named pipe", id);
      log_message(msg, 'w', 1);
      continue;
    }
    else if (bytes == 0) {
      sprintf(msg, "[Validator %d] Named pipe is closed", id);
      log_message(msg, 'w', 1);
      break;
    }

    TxBlock block = recv.block;
    int miner_id = recv.miner_id;

    sprintf(msg, "[Validator %d] Received block %s for validation from miner %d", id, block.id, miner_id);
    log_message(msg, 'r', DEBUG);

    // Validate the block
    int is_valid = 1;

    // -- Verify the block's PoW
    printf("[DEBUG] *** Verifing the block's PoW...\n");
    if (!verify_nonce(&block)) {
      is_valid = 0;
      sprintf(msg, "[Validator %d] Block %s has an invalid PoW", id, block.id);
      log_message(msg, 'w', DEBUG);
      continue;
    }

    // -- Check if the previous block hash matches the latest ledger block's hash
    printf("[DEBUG] *** Checking hash...\n");
    if (is_valid) {
      sem_wait(ledger_mutex);
      int last_block_index = -1;
      for (int i = 0; i < blockchain_blocks; i++)
        if (blocks[i].id[0] != '\0')
          last_block_index = i;

      // If the current block is not the first block on the ledger, check the hash
      if (last_block_index >= 0) {
        char last_block_hash[HASH_SIZE];
        compute_sha256(&blocks[last_block_index], last_block_hash);
        if (strcmp(block.previous_block_hash, last_block_hash) != 0) {
          is_valid = 0;
          sprintf(msg, "[Validator %d] Block %s invalid: Previous block hash does not match the last block's hash", id, block.id);
          log_message(msg, 'w', DEBUG);
        }
      }
      sem_post(ledger_mutex);
    }

    // -- Check if the transactions are still in the transactions pool
    printf("[DEBUG] *** Checking if the transactions are still in the transactions pool\n");
    if (is_valid) {
      sem_wait(tx_pool_mutex);
      for (int i = 0; i < tx_per_block; i++) {
        int found = 0;
        Tx cur_tx = block.transactions[i];
        for (int j = 0; j < tx_pool_size; j++) {
          TxPoolNode cur_node = tx_pool[j];
          if (cur_node.empty == 0 && strcmp(cur_node.tx.id, cur_tx.id) == 0) {
            found = 1;
            break;
          }
        }

        if (!found) {
          is_valid = 0;
          sprintf(msg, "[Validator %d] Block %s invalid: Transaction %s not in the pool", id, block.id, cur_tx.id);
          log_message(msg, 'w', DEBUG);
          break;
        }
      }
      increment_age(&tx_pool, tx_pool_size);  // -> Aging
      sem_post(tx_pool_mutex);
    }

    // -- Calculate the reward
    printf("[DEBUG] *** Calculating the reward\n");
    int total_reward;
    if (is_valid) {
      total_reward = get_max_transaction_reward(&block, tx_per_block);
    }

    if (is_valid) {
      // -- Place the validated block on the ledger
      sem_wait(ledger_mutex);
      int result = save_block(&blocks, &block, blockchain_blocks, tx_per_block);
      sem_post(ledger_mutex);
      if (result == 0)
        printf("[DEBUG] *** Block saved to the ledger successfully\n");
      else
        printf("[DEBUG] *** Error saving the block to the ledger\n");

      // -- Remove the block's transactions from the pool
      sem_wait(tx_pool_mutex);
      for (int i = 0; i < tx_per_block; i++)
        for (int j = 0; j < tx_pool_size; j++)
          if (tx_pool[j].empty == 0 && strcmp(tx_pool[j].tx.id, block.transactions[i].id) == 0) {
            printf("[DEBUG] *** Removing transaction %s from the pool\n", tx_pool[j].tx.id);
            tx_pool[j].empty = 1;
            sem_post(tx_pool_empty);
            break;
          }

      // -- Age the transactions in the pool
      increment_age(&tx_pool, tx_pool_size);  // -> Aging
      sem_post(tx_pool_mutex);

      sprintf(msg, "[Validator %d] Block %s validated successfully", id, block.id);
      log_message(msg, 'r', DEBUG);
    }

  }

  // Process termination
  sprintf(msg, "[Validator %d] Process terminated", id);
  log_message(msg, 'r', DEBUG);
}

/*
  Auxiliary function to copy a Block from the SRC address to an
  address in the Blockchain Ledger
*/
int save_block(TxBlock **ledger, TxBlock *src, int num_blocks, int tx_per_block) {
  // Iterate over the Blockchain Ledger and check for the first available slot
  TxBlock *block;
  for (int i = 0; i < num_blocks; i++) {
    block = ledger[i];
    if (block->id[0] == '\0') { // -> Checks if the block is empty
      // Place the SRC Block data in the Blockchain Ledger
      strcpy(block->id, src->id);
      strcpy(block->previous_block_hash, src->previous_block_hash);
      block->timestamp = src->timestamp;
      block->nonce = src->nonce;
      for (int j = 0; j < tx_per_block; j++) {
        strcpy(block->transactions[j].id, src->transactions[j].id);
        block->transactions[j].reward = src->transactions[j].reward;
        block->transactions[j].timestamp = src->transactions[j].timestamp;
        block->transactions[j].value = src->transactions[j].value;
      }
      return 0;
    }
  }
  return -1;
}