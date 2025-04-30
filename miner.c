/*
  DEIChain - Miner Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "utils.h"
#include "miner.h"
#include "structs.h"

#define DEBUG 1
#define POW_MAX_OPS 1000000



void* miner_routine(void* thread_args) {
  // Thread initialization
  struct MinerThreadArgs args = *(struct MinerThreadArgs*)thread_args;
  free(thread_args);
  int id = args.miner_id;
  int tx_per_block = args.tx_per_block;
  char msg[100];
  sprintf(msg, "[Miner] Thread %d initialized", id);
  log_message(msg, 'r', DEBUG);

  /* ---- Thread Code ---- */
  while (1) {
    //printf("[Miner Thread %d] Miner %d running...\n", id, id);
    sleep(2);
  }

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
  // "Unpack" the arguments
  int num_miners = args.num_miners;
  int tx_per_block = args.tx_per_block;
  int tx_pool_size = args.tx_pool_size;
  TxPoolNode *tx_pool = args.tx_pool;
  TxBlock *blocks;
  Tx *transactions = args.transactions;

  // Process initialization
  pthread_t thread_id[num_miners];
  char msg[100];
  sprintf(msg, "[Miner] Process initialized (PID -> %d | parent PID -> %d)", getpid(), getppid());
  log_message(msg, 'r', DEBUG);
  
  // Create the miner threads
  for (int i = 0; i < num_miners; i++) {
    struct MinerThreadArgs *thread_args = (struct MinerThreadArgs*)malloc(sizeof(struct MinerThreadArgs));
    thread_args->tx_per_block = tx_per_block;
    thread_args->miner_id = i + 1;
    if (pthread_create(&thread_id[i], NULL, miner_routine, (void*)thread_args) != 0) {
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
