/*
  DEIChain - Controller Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>

#include "utils.h"
#include "miner.h"
#include "statistics.h"
#include "validator.h"

#define DEBUG 1

/*
  DISCLAIMER:
  The process created when the executable is run by the user
  will act as the Controller process
*/


int main() {
  /*
    ----- Program Initalization
  */

  // Control variables
  int num_miners;
  int pool_size;
  int transactions_per_block;
  int blockchain_blocks;
  int transaction_pool_size;

  char msg[100];  // Variable use to temporarily store a message to be logged

  // Reading the configuration file, initializing the variables
  load_config(&num_miners, &pool_size, &transactions_per_block, &blockchain_blocks, &transaction_pool_size);
  
  sprintf(msg, "Loaded num_miners = %d", num_miners);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "Loaded pool_size = %d", pool_size);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "Loaded transactions_per_block = %d", transactions_per_block);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "Loaded blockchain_blocks = %d", blockchain_blocks);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "Loaded transaction_pool_size = %d", transaction_pool_size);
  log_message(msg, 'r', DEBUG);

  // Setting up the IPC mechanisms

  // Shared memory

  // Semaphores

  // Message queues

  // Names pipes

  // Creating the required processes
  sprintf(msg, "[Controller] Process (PID -> %d)", getpid());
  log_message(msg, 'r', DEBUG);

  // -- Miner process
  pid_t miner_pid;
  if ((miner_pid = fork()) == 0) {
    miner(num_miners);
    exit(0);
  } else if (miner_pid < 0)
    log_message("Could not create the Miner process", 'w', 1);

  // -- Validator process
  pid_t validator_pid;
  if ((validator_pid = fork()) == 0) {
    validator();
    exit(0);
  } else if (validator_pid < 0)
    log_message("Could not create the Validator process", 'w', 1);
 
  // -- Statistics process
  pid_t statistics_pid;
  if ((statistics_pid = fork()) == 0) {
    statistics();
    exit(0);
  } else if (statistics_pid < 0)
    log_message("Could not create the Statistics process", 'w', 1);
    
  // Wait for the processes to finish (Temporary approach)
  for (int i = 0; i < 3; i++)
    wait(NULL);

  return 0;
}
