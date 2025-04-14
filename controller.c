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
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "utils.h"
#include "structs.h"
#include "miner.h"
#include "statistics.h"
#include "validator.h"

#define DEBUG 1

// Semaphores and mutexes
sem_t *log_mutex;
sem_t *tx_pool_mutex;
sem_t *tx_pool_full;
sem_t *tx_pool_empty;

// Shared memory IDs
int tx_pool_id;
int blockchain_ledger_id;

// Process IDs
pid_t miner_pid, statistics_pid, validator_pid[3];

int stop_validator_manager;

/*
  Perform the cleanup of the IPC mechanisms and any active processes
*/
void signals(int signum) {
  // Perform cleanup and exit the application gracefully
  if (signum == SIGINT) {
    printf("\n");
    log_message("[Controller] ^C detected. Closing.", 'r', 1);
    // Kill any active processes
    kill(miner_pid, SIGKILL);
    kill(statistics_pid, SIGKILL);
    stop_validator_manager = 1;
    for (int i = 0; i < 3; i++) {
      if (validator_pid[i] != 0)
        kill(validator_pid[i], SIGKILL);
    }
    while (wait(NULL) != -1);

    // Detaching and removing shared memory
    if (tx_pool_id >= 0) {
      shmdt(&tx_pool_id);
      shmctl(tx_pool_id, IPC_RMID, NULL);
    }
    if (blockchain_ledger_id >= 0) {
      shmdt(&blockchain_ledger_id);
      shmctl(blockchain_ledger_id, IPC_RMID, NULL);
    }

    // Releasing semaphores and mutexes
    sem_post(log_mutex);
    sem_close(log_mutex);
    sem_unlink("LOG_MUTEX");

    exit(0);
  }

  // Calculating and printing statistics
  if (signum == SIGUSR1) {
    statistics();
  }
}

/*
  Thread routine to manage the number of Validator threads
*/
void* manage_validation(void *void_args) {
  log_message("[Controller] Validator Manager launched successfully", 'r', DEBUG);
  struct ThreadArgs args = *((struct ThreadArgs*)void_args);
  TxPoolNode *tx_pool = args.tx_pool;
  int size = args.tx_pool_size;

  // Flags
  int aux1 = 0, aux2 = 0;

  // Calculate the occupancy of the transaction pool (TODO -> implement synchronization)
  while (!stop_validator_manager) {
    sem_wait(tx_pool_mutex);
    int occupated_blocks = 0;
    for (int i = 0; i < size; i++) {
      printf("[Validator] block %2d status: %d\n", i, tx_pool[i].empty);
      if (tx_pool[i].empty == 0)
        occupated_blocks++;
    }
    int occupancy = (int)((float)occupated_blocks / size * 100);
    printf("[Controller] [Validator Manager] Current occupancy = %d\n", occupancy);
    // If the pool occupancy falls below 40%, terminate any additional validators
    if (occupancy < 40 && (aux1 == 1 || aux2 == 1)) {
      log_message("[Controller] [Validator Manager] Occupancy dropped below 40%. Terminating the additional Validator processes", 'r', DEBUG);
      if (aux1 == 1) {
        kill(validator_pid[1], SIGTERM);
        aux1 = 0;
        validator_pid[1] = 0;
      }
      if (aux2 == 1) {
        kill(validator_pid[2], SIGTERM);
        aux2 = 0;
        validator_pid[2] = 0;
      }
    }
    // If the occupancy reaches 60%, launch an auxiliary Validator
    if (occupancy >= 60 && aux1 == 0) {
      log_message("[Controller] [Validator Manager] 60% occupancy reached. Launching an additional Validator", 'r', DEBUG);
      aux1 = 1;
      if ((validator_pid[1] = fork()) == 0) {
        validator();
        exit(0);
      }
      if (validator_pid[1] < 0)
        log_message("[Controller] Error creating the 1st auxiliary validator", 'w', 1);
    }
    // If the occupancy reaches 80%, launch another auxiliary Validator
    else if (occupancy >= 80 && aux2 == 0) {
      log_message("[Controller] [Validator Manager] 80% occupancy reached. Launching an additional Validator", 'r', DEBUG);
      aux2 = 1;
      if ((validator_pid[2] = fork()) == 0) {
        validator();
        exit(0);
      }
      if (validator_pid[2] < 0)
      log_message("[Controller] Error creating the 2nd auxiliary validator", 'w', 1);
    }
    sem_post(tx_pool_mutex);
    sleep(2);
  }
  pthread_exit(NULL);
}

/*
  Main function
*/
int main() {

  // Setting up a mutex to handle access to the log file
  sem_unlink("LOG_MUTEX");
  log_mutex = sem_open("LOG_MUTEX", O_CREAT | O_EXCL, 0700, 1);

  // Process initialization
  char msg[100];  // Variable use to temporarily store a message to be logged
  sprintf(msg, "[Controller] Process initialized (PID -> %d)", getpid());
  log_message(msg, 'r', DEBUG);

  // Handle ^C signal
  signal(SIGINT, signals);

  // Handle ^T signal
  signal(SIGUSR1, signals);

  // Control variables
  int num_miners;
  int tx_pool_size;
  int tx_per_block;
  int blockchain_blocks;

  // Reading the configuration file, initializing the variables
  load_config(&num_miners, &tx_pool_size, &tx_per_block, &blockchain_blocks);
  
  sprintf(msg, "[Controller] Loaded num_miners = %d", num_miners);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded tx_pool_size = %d", tx_pool_size);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded transactions_per_block = %d", tx_per_block);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Controller] Loaded blockchain_blocks = %d", blockchain_blocks);
  log_message(msg, 'r', DEBUG);

  // Shared memory
  // -- Create the Transaction Pool's shared memory
  size_t size = sizeof(TxPoolNode) * tx_pool_size;
  key_t tx_key = ftok("config.cfg", 'K');
  if ((tx_pool_id = shmget(tx_key, size, IPC_CREAT | 0766)) < 0) {
    log_message("[Controller] Error creating the Transaction Pool (Shared Memory)", 'w', 1);
    exit(-1);
  }
  log_message("[Controller] Transaction Pool created (shared memory)", 'r', DEBUG);

  // -- Attach the Transaction Pool to the shared memory
  TxPoolNode *tx_pool;
  if ((tx_pool = (TxPoolNode*)shmat(tx_pool_id, NULL, 0)) < 0) {
		log_message("[Controller] Error attaching the Transaction Pool (Shared Memory)", 'w', 1);
		exit(-1);
	}
  log_message("[Controller] Transaction Pool attached to shared memory", 'r', DEBUG);
  printf("[Validator] shmget key: %d | shmid: %d\n", tx_key, tx_pool_id);

  // -- Initialize the Transaction Pool elements
  for (int i = 0; i < tx_pool_size; i++) {
    tx_pool[i].empty = 1;
    tx_pool[i].age = 0;
  }
  

  // -- Create the Blockchain Ledger
  size = (sizeof(TxBlock) + sizeof(Tx) * tx_per_block) * blockchain_blocks;
  if ((blockchain_ledger_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0766)) < 0) {
    log_message("[Controller] Error creating the Blockchain Ledger", 'w', 1);
    exit(-1);
  }
  log_message("[Controller] Blockchain Ledger created (shared memory)", 'r', DEBUG);

  // -- Attach the Blockchain Ledger to the shared memory
  TxBlock *blockchain_ledger;
  if ((blockchain_ledger = (TxBlock*)shmat(blockchain_ledger_id, NULL, 0)) < 0) {
    log_message("[Controller] Error attaching the Blockchain Ledger (Shared Memory)", 'w', 1);
    exit(-1);
  }
  log_message("[Controller] Blockchain Ledger attached to the shared memory", 'r', DEBUG);

  // -- Map the Blockchain Ledger elements in shared memory
  TxBlock *blocks;
  Tx *transactions;
  get_blockchain_mapping(blockchain_ledger, blockchain_blocks, &blocks, &transactions);

  // Semaphores and mutexes
  sem_unlink("TX_POOL_MUTEX");
  tx_pool_mutex = sem_open("TX_POOL_MUTEX", O_CREAT | O_EXCL, 0700, 1);
  sem_unlink("TX_POOL_FULL");
  tx_pool_full = sem_open("TX_POOL_FULL", O_CREAT | O_EXCL, 0700, tx_pool_size);
  sem_unlink("TX_POOL_EMPTY");
  tx_pool_empty = sem_open("TX_POOL_EMPTY", O_CREAT | O_EXCL, 0700, tx_pool_size);

  // Message queues

  // Named pipes

  // Creating the required processes
  // -- Miner process
  struct MinerArgs args;
  args.blocks = blocks;
  args.transactions = transactions;
  args.tx_pool = tx_pool;
  args.tx_pool_size = tx_pool_size;
  if ((miner_pid = fork()) == 0) {
    miner(num_miners, args);
    exit(0);
  } else if (miner_pid < 0) {
    log_message("[Controller] Could not create the Miner process", 'w', 1);
    exit(-1);
  }

  // -- Validator processes
  // ---- Initialize the Validator PID's array
  for (int i = 0; i < 3; i++)
    validator_pid[i] = 0;
  // ---- Create the first Validator process
  if ((validator_pid[0] = fork()) == 0) {
    validator();
    exit(0);
  } else if (validator_pid[0] < 0) {
    log_message("[Controller] Could not create the Validator process", 'w', 1);
    exit(-1);
  }
  // ---- Launch the thread to manage the transaction pool occupancy
  pthread_t validator_manager_id;
  struct ThreadArgs args;
  args.tx_pool = tx_pool;
  args.tx_pool_size = tx_pool_size;
  stop_validator_manager = 0;
  if (pthread_create(&validator_manager_id, NULL, manage_validation, (void*)&args) < 0) {
    log_message("[Controller] Error launching the thread to manage validators", 'w', 1);
    exit(-1);
  }
 
  // -- Statistics process
  if ((statistics_pid = fork()) == 0) {
    statistics();
    exit(0);
  } else if (statistics_pid < 0) {
    log_message("[Controller] Could not create the Statistics process", 'w', 1);
    exit(-1);
  }
    
  // Simulated workload
  while (1) {
    printf("[Controller] Running...\n");
    sleep(2);
  }
  
  // Wait for the processes to finish (Temporary approach)
  for (int i = 0; i < 3; i++)
    wait(NULL);
  log_message("[Controller] All subprocesses have been terminated", 'r', DEBUG);

  // Process termination
  log_message("[Controller] Process terminated", 'r', 1);
  
  return 0;
}
