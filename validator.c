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
#include <sys/msg.h>

#include "utils.h"
#include "validator.h"
#include "pow.h"

#define BUF_SIZE 200

extern int tx_per_block;
extern int tx_pool_size;
extern int blockchain_blocks;
extern TxPoolNode *tx_pool;
extern TxBlock *blocks;
extern TxBlock *blockchain_ledger;

extern sem_t *ledger_mutex;
extern sem_t *tx_pool_mutex;
extern sem_t *tx_pool_empty;
extern sem_t *pipe_mutex;
extern sem_t *hash_mutex;
extern sem_t *check_occupancy;

extern int msq_id;

extern char *last_hash;

void validator(int id) {
  // Process initialization
  signal(SIGINT, SIG_IGN);  // -> Ignore SIGINT, since auxiliary validator processes will inherit SIGINT handling
  char msg[BUF_SIZE];
  sprintf(msg, "[Validator %d] Process initialized (PID -> %d | parent PID -> %d)", id, getpid(), getppid());
  log_message(msg, 'r', DEBUG);

  int fd = open(PIPE_NAME, O_RDONLY);
  if (fd < 0) {
    sprintf(msg, "[Validator %d] Error opening the named pipe", id);
    log_message(msg, 'w', 1);
    return;
  }

  // Re-map shared memory to get consistent pointers
  TxBlock *blocks;
  char *last_hash;
  get_blockchain_mapping(blockchain_ledger, blockchain_blocks, tx_per_block, &blocks, &last_hash);

  sprintf(msg, "[Validator %d] Successfully opened the named pipe", id);
  log_message(msg, 'r', DEBUG);

  while (1) {
    PipeMsg *recv = malloc(sizeof(PipeMsg) + tx_per_block * sizeof(Tx));

    // Read a block from the named pipe (blocking state while waiting)
    int bytes = read(fd, recv, sizeof(PipeMsg) + tx_per_block * sizeof(Tx));
  
    int is_valid = 1;

    if (bytes < 0) {
      sprintf(msg, "[Validator %d] Error reading from the named pipe", id);
      log_message(msg, 'w', 1);
      free(recv);
      continue;
    }
    else if (bytes == 0) {
      sprintf(msg, "[Validator %d] Named pipe is closed", id);
      log_message(msg, 'w', 1);
      free(recv);
      break;
    }

    TxBlock block = recv->block;
    int miner_id = recv->miner_id;
    // block.transactions = NULL;
    block.transactions = malloc(tx_per_block * sizeof(Tx));
    memcpy(block.transactions, recv->transactions, tx_per_block * sizeof(Tx));

    sprintf(msg, "[Validator %d] Received block %s for validation from miner %d", id, block.id, miner_id);
    log_message(msg, 'r', DEBUG);

    // -- Verify the block's PoW
    PoWResult result;
    do {
      result = proof_of_work(&block);
    } while (result.error == 1);
    if (strcmp(recv->result_hash, result.hash) != 0) {
      is_valid = 0;
      if (DEBUG) {
        sprintf(msg, "[Validator %d] Block %s invalid: Invalid PoW", id, block.id);
        log_message(msg, 'w', 1);
      }
    }

    // Check if the previous block hash matches the hash of the last block added to the ledger
    if (is_valid) {
      // -- If the current block is not the first block on the ledger, check the hash
      sem_wait(hash_mutex);
      if (last_hash[0] != '\0')
        if (strcmp(last_hash, block.previous_block_hash) != 0) {
          is_valid = 0;
          sprintf(msg, "[Validator %d] Block %s invalid: Previous block hash does not match the last block's hash", id, block.id);
          log_message(msg, 'w', 1);
        }
      sem_post(hash_mutex);
    }

    // -- Check if the transactions are still in the transactions pool
    if (is_valid) {
      sem_wait(tx_pool_mutex);
      for (int i = 0; i < tx_per_block; i++) {
        int found = 0;
        Tx cur_tx = block.transactions[i];
        for (int j = 0; j < tx_pool_size; j++) {
          TxPoolNode *cur_node = &tx_pool[j];
          if (cur_node->empty == 0 && strcmp(cur_node->tx.id, cur_tx.id) == 0) {
            found = 1;
            break;
          }
        }

        if (!found) {
          is_valid = 0;
          sprintf(msg, "[Validator %d] Block %s invalid: Transaction %s not in the pool", id, block.id, cur_tx.id);
          log_message(msg, 'w', 1);
          break;
        }
      }
      increment_age(tx_pool, tx_pool_size);  // -> Aging
      sem_post(tx_pool_mutex);
    }

    // -- Calculate the reward
    int total_reward = 0;
    if (is_valid)
      total_reward = get_max_transaction_reward(&block, tx_per_block);

    if (is_valid) {
      // -- Place the validated block on the ledger
      sem_wait(ledger_mutex);
      int saved = save_block(&blocks, &block);
      sem_post(ledger_mutex);
      if (saved) {
        sprintf(msg, "[Validator %d] Block %s added to the ledger", id, block.id);
        log_message(msg, 'r', DEBUG);
      }
      else {
        sprintf(msg, "[Validator %d] Error saving block %s to the ledger", id, block.id);
        log_message(msg, 'w', 1);
      }

      // -- Remove the block's transactions from the pool
      sem_wait(tx_pool_mutex);
      for (int i = 0; i < tx_per_block; i++)
        for (int j = 0; j < tx_pool_size; j++)
          if (tx_pool[j].empty == 0 && strcmp(tx_pool[j].tx.id, block.transactions[i].id) == 0) {
            // printf("[DEBUG] [Validator] *** Removing transaction %s from the pool\n", tx_pool[j].tx.id);
            tx_pool[j].empty = 1;
            sem_post(tx_pool_empty);
            break;
          }

      // -- Age the transactions in the pool
      increment_age(tx_pool, tx_pool_size);  // -> Aging
      sem_post(tx_pool_mutex);

      // Save the hash of the current block for future validation of the previous block hash
      sem_wait(hash_mutex);
      strcpy(last_hash, result.hash);
      sem_post(hash_mutex);
      sprintf(msg, "[Validator %d] Block %s validated successfully", id, block.id);
      log_message(msg, 'r', DEBUG);
      sem_post(check_occupancy);  // -> Unblock the Validator Manager to check the pool's occupancy
    }

    // Send the results to the statistics process
    Message to_send;
    to_send.miner_id = miner_id;
    to_send.msgtype = 1;
    to_send.valid_block = is_valid;
    if (is_valid) {
      to_send.creation_time = recv->block.timestamp;
      to_send.validation_time = get_timestamp();
      to_send.credits = total_reward;
    }
    msgsnd(msq_id, &to_send, sizeof(Message) - sizeof(long), 0);

    free(block.transactions);
    free(recv);
  } // -> while (1)

  // Process termination
  sprintf(msg, "[Validator %d] Process terminated", id);
  log_message(msg, 'r', DEBUG);
}

/*
  Auxiliary function to copy a Block from the SRC address to an
  address in the Blockchain Ledger
*/
int save_block(TxBlock **ledger, TxBlock *src) {
  TxBlock *blocks = *ledger;
  // Iterate over the Blockchain Ledger and check for the first available slot
  for (int i = 0; i < blockchain_blocks; i++) {
    TxBlock *block = &blocks[i];
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
      return 1;
    }
  }
  return 0;
}