/*
  DEIChain - Miner Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>

#include "utils.h"
#include "miner.h"
#include "structs.h"
#include "pow.h"

#define BUF_SIZE 200

extern int num_miners;
extern int tx_per_block;
extern int tx_pool_size;
extern TxPoolNode *tx_pool;
extern TxBlock *blocks;

extern sem_t *tx_pool_mutex;
extern sem_t *min_tx;
extern sem_t *pipe_mutex;
sem_t *console_mutex;

void* miner_routine(void* miner_id) {
  // Thread initialization
  int id = *(int*)miner_id;
  char msg[BUF_SIZE];
  sprintf(msg, "[Miner] Thread %d initialized", id);
  log_message(msg, 'r', DEBUG);

  // Miner thread routine
  int block_count = 0;
  char prev_hash[HASH_SIZE];
  while (1) {
    // -- Assemble a new block
    TxBlock block;
    char buf[64];
    sprintf(buf, "BLOCK-%lu-%d", pthread_self(), block_count);
    strcpy(block.id, buf);
    if (block_count == 0)
      strcpy(block.previous_block_hash, INITIAL_HASH);
    else
      strcpy(block.previous_block_hash, prev_hash);
    // -- Fill the block with transactions
    block.transactions = (Tx*)malloc(sizeof(Tx)*tx_per_block);
    
    // -- Check the available transactions
    sem_wait(min_tx);
    sem_wait(tx_pool_mutex);

    // -- Select transactions from the Transactions Pool
    //sem_wait(console_mutex);
    if (DEBUG)
      printf("[Miner Thread %d] Assembling block\n", id);
    int num_selected = 0;
    for (int i = 0; i < tx_per_block; i++) {
      for (int j = 0; j < tx_pool_size; j++) {
        TxPoolNode *cur = &tx_pool[j];
        if (cur->empty == 1) // -> Skip empty slots
          continue;

        // -- Miners with even thread ID => prioritize easier transactions
        if (/*id % 2 == */0) {
          if (getDifficultFromReward(cur->tx.reward) == EASY) {
            strcpy(block.transactions[i].id, cur->tx.id);
            block.transactions[i].reward = cur->tx.reward;
            block.transactions[i].timestamp = cur->tx.timestamp;
            block.transactions[i].value = cur->tx.value;
            cur->selected = 1;
            num_selected++;
            break;
          }
        }
        // -- Miners with odd thread ID => prioritize selecting sequential transactions
        else {

        }
      }
    }
    // -- Select remaining transactions if there are empty slots in the block
    if (num_selected < tx_per_block) {
      for (int i = num_selected; i < tx_per_block; i++) {
        for (int j = 0; j < tx_pool_size; j++) {
          TxPoolNode *cur = &tx_pool[j];
          if (cur->empty == 1) // -> Skip empty slots
            continue;
          if (cur->selected == 0) {
            strcpy(block.transactions[i].id, cur->tx.id);
            block.transactions[i].reward = cur->tx.reward;
            block.transactions[i].timestamp = cur->tx.timestamp;
            block.transactions[i].value = cur->tx.value;
            cur->selected = 1;
            num_selected++;
            break;
          }
        }
      }
    }
    // -- Reset the selected flag in all nodes of the transaction pool
    for (int i = 0; i < tx_pool_size; i++)
      tx_pool[i].selected = 0;
    sem_post(tx_pool_mutex);

    block.timestamp = get_timestamp();  // -> Assign the timestamp of the instant the block's assembly is completed

    // -- Print the block (DEBUG)
    // print_block(block, tx_per_block);
    // sem_post(console_mutex);

    // Get the miner to mine the assembled block
    sprintf(msg, "[Miner Thread %d] Started mining block %s", id, block.id);
    log_message(msg, 'r', DEBUG);

    PoWResult result;
    do {
      block.timestamp = get_timestamp();
      result = proof_of_work(&block); // -> Find a valid nonce
    } while (result.error == 1);

    // If the number of operations reaches the limit
    /*
    if (result.error) {
      sprintf(msg, "[Miner Thread %d] Failed to mine block %s: max operations reached", id, block.id);
      log_message(msg, 'w', DEBUG);
      free(block.transactions); // -> Free allocated memory
      continue;                 // -> Assemble a new block and try again
    }
    */

    // If the mining process succeeds
    sprintf(msg, "[Miner Thread %d] Successfully mined block %s", id, block.id);
    log_message(msg, 'r', DEBUG);
    strcpy(prev_hash, result.hash);

    // Send the block to the validators via Named Pipe
    PipeMsg block_data;
    block_data.miner_id = id;
    block_data.block = block;

    sem_wait(pipe_mutex);
    int fd = open(PIPE_NAME, O_WRONLY);
    if (fd < 0) {
      sprintf(msg, "[Miner Thread %d] Error opening the named pipe", id);
      log_message(msg, 'w', 1);
    }

    write(fd, &block_data, sizeof(PipeMsg)); // -> Send the block data
    close(fd);
    sem_post(pipe_mutex);

    sprintf(msg, "[Miner Thread %d] Sent block %s for validation", id, block.id);
    log_message(msg, 'r', DEBUG);

    // Prepare the assembly of the next block
    block_count++;
    free(block.transactions);
  } // -> while (1)
  
  // Thread termination
  sprintf(msg, "[Miner] Thread %d terminated", id);
  log_message(msg, 'r', DEBUG);
  pthread_exit(NULL);
}

/*
  Operates a determined number of miner threads passed as an
  argument. Each miner thread will simulate an individual miner,
  reading transactions, grouping them into blocks and performing
  a PoW step.
*/
void miner(struct MinerArgs args) {
  // Process initialization
  pthread_t thread_id[num_miners];
  char msg[100];
  sprintf(msg, "[Miner] Process initialized (PID -> %d | parent PID -> %d)", getpid(), getppid());
  log_message(msg, 'r', DEBUG);

  // Open the named semaphore
  tx_pool_mutex = sem_open("TX_POOL_MUTEX", 0);
  if (tx_pool_mutex == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m tx_pool_mutex not initialized yet. Closing.\n");
    exit(-1);
  }
  console_mutex = sem_open("CONSOLE_MUTEX", O_CREAT, 0700, 1);
  
  // Create the miner threads
  int miner_id[num_miners];
  for (int i = 0; i < num_miners; i++) {
    miner_id[i] = i + 1;
    if (pthread_create(&thread_id[i], NULL, miner_routine, (void*)&miner_id[i]) < 0) {
      sprintf(msg, "[Miner] Error creating miner thread %d", i + 1);
      log_message(msg, 'w', 1);
      exit(-1);
    }
  }

  // Wait for the threads to finish running
  for (int i = 0; i < num_miners; i++) {
    if (pthread_join(thread_id[i], NULL) != 0) {
      sprintf(msg, "[Miner] Error joining miner thread %d", i + 1);
      log_message(msg, 'w', 1);
      exit(-1);
    }
  }

  // Process termination
  log_message("[Miner] Process terminated", 'r', 1);
}
