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

#include "utils.h"
#include "miner.h"

#define DEBUG 1


void* miner_routine(void* miner_id) {
  int id = *((int*)miner_id);
  char msg[100];
  sprintf(msg, "[Miner] Thread %d initialized!", id);
  log_message(msg, 'r', DEBUG);
  pthread_exit(NULL);
}

/*
  Operates a determined number of miner threads passed as an
  argument. Each miner thread will simulate an individual miner,
  reading transactions, grouping them into blocks and performing
  a PoW step.
*/
void miner(int num_miners) {
  pthread_t thread_id[num_miners];
  int miner_id[num_miners];
  char msg[100];

  sprintf(msg, "[Miner] Process initialized (parent PID -> %d)", getppid());
  log_message(msg, 'r', DEBUG);
  
  // Create the miner threads
  for (int i = 0; i < num_miners; i++) {
    miner_id[i] = i + 1;
    if (pthread_create(&thread_id[i], NULL, miner_routine, &miner_id[i]) != 0) {
      sprintf(msg, "Error creating miner thread %d", miner_id[i]);
      log_message(msg, 'w', 1);
      exit(-1);
    }
  }

  // Wait for the threads to finish running
  for (int i = 0; i < num_miners; i++) {
    if (pthread_join(thread_id[i], NULL) != 0) {
      sprintf(msg, "Error joining miner thread %d", miner_id[i]);
      log_message(msg, 'w', 1);
      exit(-1);
    }
  }

}
