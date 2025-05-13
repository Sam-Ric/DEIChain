/*
  DEIChain - Controller Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#define _POSIX_C_SOURCE 200809L // Signal library MACRO fix

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
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

// Semaphores and mutexes
sem_t *log_mutex;         // Mutex to control writing to the log file
sem_t *tx_pool_mutex;     // Mutex to control access to the Transactions Pool
sem_t *tx_pool_full;      // Semaphore to control occupied slots in the Transactions Pool
sem_t *tx_pool_empty;     // Semaphore to control available slots in the Transactions Pool
sem_t *ledger_mutex;      // Mutex to control access to the Blockchain Ledger
sem_t *pipe_mutex;        // Mutex to control access to the named pipe
sem_t *hash_mutex;        // Mutex to control access to the hash of the last validated block
sem_t *stats_done;        // Semaphore to block other processes while the statistics are being printed
sem_t *check_occupancy;   // Semaphore to avoid busy waiting on the Validator Manager thread

// Shared memory IDs
int tx_pool_id;               // ID of the Transaction Pool's shared memory
int blockchain_ledger_id;     // ID of the Blockchain Ledger's shared memory
TxPoolNode *tx_pool;          // Transactions Pool shared memory pointer (structs array)
TxBlock *blockchain_ledger;   // Blockchain Ledger shared memory pointer (not mapped)
TxBlock *blocks;              // Blockchain Ledger shared memory pointer (mapped)

int msq_id;        // Message queue ID

int handling_sigusr1 = 0;

// Process IDs
pid_t controller_pid, miner_pid, statistics_pid, validator_pid[3];

// Global variables
int num_miners;                   // Number of miner threads
int tx_pool_size;                 // Number of transaction slots in the Transactions Pool
int tx_per_block;                 // Number of transactions per block
int blockchain_blocks;            // Number of block slots in the Blockchain Ledger
int stop_validator_manager;       // Flag to stop the validator manager
FILE *log_file;                   // File pointer of the log file
char *last_hash;                  // String containing the hash of the last block added to the ledger

void cleanup() {
  // Close the log file
  fclose(log_file);

  // Detaching and removing shared memory
  if (tx_pool_id >= 0) {
    shmdt(tx_pool);
    shmctl(tx_pool_id, IPC_RMID, NULL);
  }
  if (blockchain_ledger_id >= 0) {
    shmdt(blockchain_ledger);
    shmctl(blockchain_ledger_id, IPC_RMID, NULL);
  }

  // Removing the named pipe
  unlink(PIPE_NAME);

  // Removing the message queue
  msgctl(msq_id, IPC_RMID, NULL);

  // Releasing semaphores and mutexes
  sem_close(log_mutex);
  sem_close(tx_pool_empty);
  sem_close(tx_pool_full);
  sem_close(tx_pool_mutex);
  sem_close(ledger_mutex);
  sem_close(pipe_mutex);
  sem_close(hash_mutex);
  sem_close(stats_done);
  sem_close(check_occupancy);
  sem_unlink("LOG_MUTEX");
  sem_unlink("TX_POOL_EMPTY");
  sem_unlink("TX_POOL_FULL");
  sem_unlink("TX_POOL_MUTEX");
  sem_unlink("LEDGER_MUTEX");
  sem_unlink("PIPE_MUTEX");
  sem_unlink("HASH_MUTEX");
  sem_unlink("STATS_DONE");
  sem_unlink("CHECK_OCCUPANCY");
}

/*
  Perform the cleanup of the IPC mechanisms and any active processes
*/
void signals(int signum) {
  // Perform cleanup and exit the application gracefully
  if (signum == SIGINT) {
    printf("\n");
    log_message("[Controller] SIGINT received. Closing.", 'r', 1);

    // Printing simulation statistics
    kill(statistics_pid, SIGUSR1);
    sem_wait(stats_done);

    // Properly close the pipe to unblock any readers
    int fd = open(PIPE_NAME, O_WRONLY | O_NONBLOCK);
    if (fd >= 0) {
      // Write something to unblock readers if needed
      char dummy = 0;
      write(fd, &dummy, 0);  // Zero-length write
      close(fd);
    }

    // Kill any active processes
    kill(miner_pid, SIGKILL);
    kill(statistics_pid, SIGKILL);
    stop_validator_manager = 1;
    log_message("[Controlled] Waiting for subprocesses to finish executing", 'r', 1);
    for (int i = 0; i < 3; i++) {
      if (validator_pid[i] != 0)
        kill(validator_pid[i], SIGKILL);
    }
    while (wait(NULL) != -1);

    // Dumping the Blockchain Ledger
    log_message("[Controller] Dumping the Blockchain Ledger", 'r', 1);
    dump_ledger(blocks, blockchain_blocks, tx_per_block);

    // Cleanup IPCs and semaphores
    cleanup();

    exit(0);
  }

  if (signum == SIGUSR1) {
    if (handling_sigusr1)
      return;
    handling_sigusr1 = 1;
    kill(statistics_pid, SIGUSR1);
    sem_wait(stats_done);
    handling_sigusr1 = 0;
  }
}

/*
  Thread routine to manage the number of Validator threads
*/
void* manage_validation(void *args) {
  log_message("[Controller] Validator Manager launched successfully", 'r', DEBUG);
  TxPoolNode *tx_pool = (TxPoolNode*)args;
  int size = tx_pool_size;

  // Flags
  int aux1 = 0, aux2 = 0;

  // Calculate the occupancy of the transaction pool (TODO -> implement synchronization)
  while (!stop_validator_manager) {
    sem_wait(check_occupancy); // -> Block until there is the need to check the pool's occupancy
    sem_wait(tx_pool_mutex);
    int occupated_blocks = 0;
    for (int i = 0; i < size; i++) {
      // -- Debug messages to check the status of the Transactions Pool
      /*
      if (tx_pool[i].empty == 0)
        printf("[Validator] Block %2d status: %s\n", i, tx_pool[i].tx.id);
      else
        printf("[Validator] Block %2d status: EMPTY\n", i);
      */
      if (tx_pool[i].empty == 0)
        occupated_blocks++;
    }
    
    // When enough transactions are available
    if (occupated_blocks >= tx_per_block) {
      if (DEBUG)
        printf("    [Controller] [Validator Manager] Signaling the miner threads\n");
      // Signal all miner threads
      kill(miner_pid, SIGUSR2);
    }
    // When transactions drop below threshold => Reset the flag
    else if (DEBUG)
      printf("    [Controller] [Validator Manager] *** Not enough transactions in the pool ***\n");

    int occupancy = (int)((float)occupated_blocks / size * 100);
    if (DEBUG)
      printf("    [Controller] [Validator Manager] Current occupancy = %d%%\n", occupancy);
    // If the pool occupancy falls below 40%, terminate any additional validators
    if (occupancy < 40 && (aux1 == 1 || aux2 == 1)) {
      log_message("[Controller] [Validator Manager] Occupancy dropped below 40%. Terminating the additional Validator processes", 'r', DEBUG);
      if (aux1 == 1) {
        kill(validator_pid[1], SIGKILL);
        aux1 = 0;
        validator_pid[1] = 0;
      }
      if (aux2 == 1) {
        kill(validator_pid[2], SIGKILL);
        aux2 = 0;
        validator_pid[2] = 0;
      }
    }
    // If the occupancy reaches 60%, launch an auxiliary Validator
    if (occupancy >= 60 && aux1 == 0) {
      log_message("[Controller] [Validator Manager] 60% occupancy reached. Launching an additional Validator", 'r', DEBUG);
      aux1 = 1;
      if ((validator_pid[1] = fork()) == 0) {
        validator(2);
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
        validator(3);
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

  // -- Open the log file for writing
  log_file = fopen("DEIChain_log.txt", "a");
  if (log_file == NULL) {
    printf("\x1b[31m[!]\x1b[0m [Controller] Error opening the log file\n");
    exit(-1);
  }

  controller_pid = getpid();

  // Process initialization
  char cover[] = 
        " /$$$$$$$  /$$$$$$$$ /$$$$$$  /$$$$$$  /$$                 /$$          \n"
        "| $$__  $$| $$_____/|_  $$_/ /$$__  $$| $$                |__/          \n"
        "| $$  \\ $$| $$        | $$  | $$  \\__/| $$$$$$$   /$$$$$$  /$$ /$$$$$$$ \n"
        "| $$  | $$| $$$$$     | $$  | $$      | $$__  $$ |____  $$| $$| $$__  $$\n"
        "| $$  | $$| $$__/     | $$  | $$      | $$  \\ $$  /$$$$$$$| $$| $$  \\ $$\n"
        "| $$  | $$| $$        | $$  | $$    $$| $$  | $$ /$$__  $$| $$| $$  | $$\n"
        "| $$$$$$$/| $$$$$$$$ /$$$$$$|  $$$$$$/| $$  | $$|  $$$$$$$| $$| $$  | $$\n"
        "|_______/ |________/|______/ \\______/ |__/  |__/ \\_______/|__/|__/  |__/\n";
    
  printf("\n%s\n", cover);
  printf(" Welcome to the DEIChain blockchain simulation!\n\n");

  char msg[100];  // Variable use to temporarily store a message to be logged
  sprintf(msg, "[Controller] Process initialized (PID -> %d)", getpid());
  log_message(msg, 'r', DEBUG);

  // Ignore the signals until everything is properly initialized
  struct sigaction act;
  act.sa_handler = SIG_IGN;
  for (int i = 0; i < _NSIG; i++)
    sigaction(i, &act, NULL);

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
    cleanup();
    exit(-1);
  }
  log_message("[Controller] Transaction Pool created (shared memory)", 'r', DEBUG);

  // -- Attach the Transaction Pool to the shared memory
  if ((tx_pool = (TxPoolNode*)shmat(tx_pool_id, NULL, 0)) < 0) {
		log_message("[Controller] Error attaching the Transaction Pool (Shared Memory)", 'w', 1);
    cleanup();
		exit(-1);
	}
  log_message("[Controller] Transaction Pool attached to shared memory", 'r', DEBUG);

  // -- Initialize the Transaction Pool elements
  for (int i = 0; i < tx_pool_size; i++) {
    tx_pool[i].empty = 1;
    tx_pool[i].age = 0;
  }
  

  // -- Create the Blockchain Ledger
  size = (sizeof(TxBlock) + sizeof(Tx) * tx_per_block) * blockchain_blocks + HASH_SIZE;
  if ((blockchain_ledger_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0766)) < 0) {
    log_message("[Controller] Error creating the Blockchain Ledger", 'w', 1);
    cleanup();
    exit(-1);
  }
  log_message("[Controller] Blockchain Ledger created (shared memory)", 'r', DEBUG);

  // -- Attach the Blockchain Ledger to the shared memory
  if ((blockchain_ledger = (TxBlock*)shmat(blockchain_ledger_id, NULL, 0)) < 0) {
    log_message("[Controller] Error attaching the Blockchain Ledger (Shared Memory)", 'w', 1);
    cleanup();
    exit(-1);
  }
  log_message("[Controller] Blockchain Ledger attached to the shared memory", 'r', DEBUG);

  // -- Map the Blockchain Ledger elements in shared memory
  get_blockchain_mapping(blockchain_ledger, blockchain_blocks, tx_per_block, &blocks, &last_hash);

  // -- Initialize the Ledger's blocks
  Timestamp t;
  t.hour = 0;
  t.min = 0;
  t.sec = 0;
  for (int i = 0; i < blockchain_blocks; i++) {
    blocks[i].id[0] = '\0';
    blocks[i].previous_block_hash[0] = '\0';
    blocks[i].timestamp = t;
    blocks[i].nonce = 0;
  }
  last_hash[0] = '\0';

  // Create semaphores and mutexes
  sem_unlink("TX_POOL_MUTEX");
  tx_pool_mutex = sem_open("TX_POOL_MUTEX", O_CREAT | O_EXCL, 0700, 1);
  sem_unlink("TX_POOL_FULL");
  tx_pool_full = sem_open("TX_POOL_FULL", O_CREAT | O_EXCL, 0700, 0);
  sem_unlink("TX_POOL_EMPTY");
  tx_pool_empty = sem_open("TX_POOL_EMPTY", O_CREAT | O_EXCL, 0700, tx_pool_size);
  sem_unlink("LEDGER_MUTEX");
  ledger_mutex = sem_open("LEDGER_MUTEX", O_CREAT | O_EXCL, 0700, 1);
  sem_unlink("PIPE_MUTEX");
  pipe_mutex = sem_open("PIPE_MUTEX", O_CREAT | O_EXCL, 0700, 1);
  sem_unlink("HASH_MUTEX");
  hash_mutex = sem_open("HASH_MUTEX", O_CREAT | O_EXCL, 0700, 1);
  sem_unlink("STATS_DONE");
  stats_done = sem_open("STATS_DONE", O_CREAT | O_EXCL, 0700, 0);
  sem_unlink("CHECK_OCCUPANCY");
  check_occupancy = sem_open("CHECK_OCCUPANCY", O_CREAT | O_EXCL, 0700, 0);

  // Create the message queue
  key_t msq_key = ftok("config.cfg", 'M');
  if ((msq_id = msgget(msq_key, IPC_CREAT | IPC_EXCL | 0766)) < 0) {
    log_message("[Controller] Error creating the message queue", 'w', 1);
    cleanup();
    exit(-1);
  }

  // Create the named pipe
  if (mkfifo(PIPE_NAME, O_CREAT | O_EXCL | 0766) < 0) {
    log_message("[Controller] Error creating the named pipe", 'w', 1);
    cleanup();
    exit(-1);
  }

  // Creating the required processes
  // -- Miner process
  if ((miner_pid = fork()) == 0) {
    miner();
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
    validator(1);
    exit(0);
  } else if (validator_pid[0] < 0) {
    log_message("[Controller] Could not create the Validator process", 'w', 1);
    exit(-1);
  }
  // ---- Launch the thread to manage the transaction pool occupancy
  pthread_t validator_manager_id;
  stop_validator_manager = 0;
  if (pthread_create(&validator_manager_id, NULL, manage_validation, (void*)tx_pool) < 0) {
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

  // Handle the signals accordingly
  act.sa_handler = signals;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGUSR1, &act, NULL);
  
  // Wait for the processes to finish
  for (int i = 0; i < 3; i++)
    wait(NULL);
  log_message("[Controller] All subprocesses have been terminated", 'r', DEBUG);

  // Process termination
  log_message("[Controller] Process terminated", 'r', 1);
  
  return 0;
}
