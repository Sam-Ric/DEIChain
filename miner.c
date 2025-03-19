/*
  DEIChain - Miner
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023206471)
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "header.h"

#define DEBUG

/*

*/
void* miner_routine(void* miner_id) {
  int id = *((int*)miner_id);
  #ifdef DEBUG
    printf("[DEBUG] [Miner] Thread %d initialized!\n", id);
  #endif
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

  #ifdef DEBUG
    printf("[DEBUG] [Miner] Process initialized (parent PID -> %d)\n", getppid());
  #endif
  
  // Create the miner threads
  for (int i = 0; i < num_miners; i++) {
    miner_id[i] = i + 1;
    if (pthread_create(&thread_id[i], NULL, miner_routine, &miner_id[i]) != 0)
      erro("\x1b[31m[!]\x1b[0m Error creating miner thread");
  }

  // Wait for the threads to finish running
  for (int i = 0; i < num_miners; i++) {
    if (pthread_join(thread_id[i], NULL) != 0)
      erro("\x1b[31m[!]\x1b[0m Error joining miner thread");
  }


}