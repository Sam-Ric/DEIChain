/*
  DEIChain - Controller
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <wait.h>

#include "header.h"

#define DEBUG
#define BUFFER_SIZE 100

/*
  DISCLAIMER:
  The process created when the executable is run by the user
  will act as the Controller process
*/

void erro(char *msg) {
  perror(msg);
  exit(1);
}

/*
  Initializes the variables passed as arguments with the values from the
  configuration file 'config.cfg'
*/
void load_config(int *num_miners, int *pool_size, int *transactions_per_block, int *blockchain_blocks, int *transaction_pool_size) {
  // Open the file
  char buffer[BUFFER_SIZE];
  FILE *config_file;
  if ((config_file = fopen("config.cfg", "r")) == NULL)
    erro("\x1b[31m[!]\x1b[0m Could not load the configuration file");

  int line = 0;
  while (line < 4) {
    if (fgets(buffer, BUFFER_SIZE, config_file) == NULL) {
      printf("\x1b[31m[!]\x1b[0m Invalid configuration file\n");
      exit(1);
    }

    buffer[strcspn(buffer, "\n")] = '\0';   // Remove the '\n' character
    
    // Assign the read value to the correct variable
    if (line == 0 && (*num_miners = atoi(buffer)) == 0) {
      printf("\x1b[31m[!]\x1b[0m Invalid value for NUM_MINERS\n");
      exit(1);
    } else if (line == 1 && (*pool_size = atoi(buffer)) == 0) {
      printf("\x1b[31m[!]\x1b[0m Invalid value for POOL_SIZE\n");
      exit(1);
    } else if (line == 2 && (*transactions_per_block = atoi(buffer)) == 0) {
      printf("\x1b[31m[!]\x1b[0m Invalid value for TRANSACTIONS_PER_BLOCK\n");
      exit(1);
    } else if (line == 3 && (*blockchain_blocks = atoi(buffer)) == 0) {
      printf("\x1b[31m[!]\x1b[0m Invalid value for BLOCKCHAIN_BLOCKS\n");
      exit(1);
    }
    line++;
  }

  // Assign a value to the TRANSACTION_POOL_SIZE variable
  // -- If there is a custom value in the file, assign that value
  if (fgets(buffer, BUFFER_SIZE, config_file) != NULL) {
    if ((*transaction_pool_size = atoi(buffer)) == 0) {
      printf("\x1b[31m[!]\x1b[0m Invalid value for TRANSACTION_POOL_SIZE\n");
      exit(1);
    }
  }
  // Else, use the default value of 10000
  else
    *transaction_pool_size = 10000;
}


/*
  Perform the cleanup of the IPC mechanisms
*/
void cleanup() {

}

int main() {
  /*
    ----- Program Initalization
  */

  int num_miners;
  int pool_size;
  int transactions_per_block;
  int blockchain_blocks;
  int transaction_pool_size;

  // Reading the configuration file, initializing the variables
  load_config(&num_miners, &pool_size, &transactions_per_block, &blockchain_blocks, &transaction_pool_size);
  
  #ifdef DEBUG
    printf("[DEBUG] num_miners = %d\n", num_miners);
    printf("[DEBUG] pool_size = %d\n", pool_size);
    printf("[DEBUG] transactions_per_block = %d\n", transactions_per_block);
    printf("[DEBUG] blockchain_blocks = %d\n", blockchain_blocks);
    printf("[DEBUG] transaction_pool_size = %d\n", transaction_pool_size);
  #endif

  // Setting up the IPC mechanisms

  // Shared memory

  // Semaphores

  // Message queues

  // Names pipes

  // Creating the required processes
    #ifdef DEBUG
      printf("[DEBUG] [Controller] Process (PID -> %d)\n", getpid());
    #endif

  // -- Miner process
  pid_t miner_pid;
  if ((miner_pid = fork()) == 0) {
    miner(num_miners);
    exit(0);
  } else if (miner_pid < 0)
    erro("\x1b[31m[!]\x1b[0m Could not create the Miner process");

  // -- Validator process
  pid_t validator_pid;
  if ((validator_pid = fork()) == 0) {
    validator();
    exit(0);
  } else if (validator_pid < 0)
    erro("\x1b[31m[!]\x1b[0m Could not create the Validator process");

  // -- Statistics process
  pid_t statistics_pid;
  if ((statistics_pid = fork()) == 0) {
    statistics();
    exit(0);
  } else if (statistics_pid < 0)
    erro("\x1b[31m[!]\x1b[0m Could not create the Statistics process");

  // Wait for the processes to finish (Temporary approach)
  for (int i = 0; i < 3; i++)
    wait(NULL);

  return 0;
}
