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

// Shared memory IDs
int transactions_pool_id;
int blockchain_ledger_id;

// Process IDs
pid_t process_id[3];


/*
  Perform the cleanup of the IPC mechanisms and any active processes
*/
/*
void cleanup() {
  // Kill any active processes
  int i = 0;
  int lim = sizeof(process_id)/sizeof(pid_t);
  while (i < lim) {
    kill(process_id[i++], SIGKILL);
  }
  while (wait(NULL) != -1);
}
*/

int main() {
  // Process initialization
  char msg[100];  // Variable use to temporarily store a message to be logged
  sprintf(msg, "[Controller] Process initialized (PID -> %d)", getpid());
  log_message(msg, 'r', DEBUG);

  // Control variables
  int num_miners;
  int pool_size;
  int transactions_per_block;
  int blockchain_blocks;
  int transaction_pool_size;

  // Reading the configuration file, initializing the variables
  load_config(&num_miners, &pool_size, &transactions_per_block, &blockchain_blocks, &transaction_pool_size);
  
  sprintf(msg, "[Controller] Loaded num_miners = %d", num_miners);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded pool_size = %d", pool_size);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded transactions_per_block = %d", transactions_per_block);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded blockchain_blocks = %d", blockchain_blocks);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded transaction_pool_size = %d", transaction_pool_size);
  log_message(msg, 'r', DEBUG);

  // Setting up the IPC mechanisms

  // Shared memory
  /*
  // -- Create the Transactions Pool
  if ((transactions_pool_id = shmget(IPC_PRIVATE, TODO -> transaction size, IPC_CREAT | 0766)) < 0) {
    log_message("[Controller] Error creating the Transactions Pool (Shared Memory)", 'w', 1);
    exit(-1);
  }
  // -- Create the Blockchain Ledger
  if ((blockchain_ledger_id = shmget(IPC_PRIVATE,  TODO -> blockchain ledger size, IPC_CREAT | 0766)) < 0) {
    log_message("[Controller] Error creating the Blockchain Ledger", 'w', 1);
    exit(-1);
  }
  */

  // Semaphores

  // Message queues

  // Named pipes

  // Creating the required processes
  // -- Miner process
  if ((process_id[0] = fork()) == 0) {
    miner(num_miners);
    exit(0);
  } else if (process_id[0] < 0)
    log_message("[Controller] Could not create the Miner process", 'w', 1);

  // -- Validator process
  if ((process_id[1] = fork()) == 0) {
    validator();
    exit(0);
  } else if (process_id[1] < 0)
    log_message("[Controller] Could not create the Validator process", 'w', 1);
 
  // -- Statistics process
  if ((process_id[2] = fork()) == 0) {
    statistics();
    exit(0);
  } else if (process_id[2] < 0)
    log_message("[Controller] Could not create the Statistics process", 'w', 1);
    
  // Wait for the processes to finish (Temporary approach)
  for (int i = 0; i < 3; i++)
    wait(NULL);
  log_message("[Controller] All subprocesses have been terminated", 'r', DEBUG);

  // Process termination
  log_message("[Controller] Process terminated", 'r', 1);
  return 0;
}
