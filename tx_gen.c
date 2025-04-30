/*
  DEIChain - Transaction Generator Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <semaphore.h>

#include "utils.h"
#include "structs.h"

#define DEBUG 1

FILE *log_file;

int main(int argc, char *argv[]) {
  // Open the log file
  log_file = fopen("DEIChain_log.txt", "a");
  if (log_file == NULL) {
    printf("\x1b[31m[!]\x1b[0m [TxGen] Error opening the log file\n");
    exit(-1);
  }

  // Verify the given arguments
  if (argc != 3) {
    log_message("[Tx Gen] Error creating a Transaction Generator", 'w', 1);
    printf("Correct format: TxGen <reward> <sleeptime>\n");
    exit(-1);
  }

  // Assign the arguments to the respective variables
  int reward;
  int sleeptime;
  int error_flag = 0;
  if ((reward = convert_to_int(argv[1])) == 0 || reward < 1 || reward > 3) {
    log_message("[Tx Gen] Invalid reward parameter", 'w', 1);
    printf("reward: 1 to 3\n");
    error_flag = 1;
  }
  if ((sleeptime = convert_to_int(argv[2])) == 0 || sleeptime < 200 || sleeptime > 3000) {
    log_message("[Tx Gen] Invalid sleep time parameter", 'w', 1);
    printf("sleeptime (ms): 200 to 3000\n");
    error_flag = 1;
  }
  if (error_flag)
    exit(-1);

  // Process initialization
  char msg[100];
  sprintf(msg, "[Tx Gen] [PID %d] Process initialized", getpid());
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Tx Gen] [PID %d] reward = %d", getpid(), reward);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Tx Gen] [PID %d] sleeptime = %d", getpid(), sleeptime);
  log_message(msg, 'r', DEBUG);

  // Open the Transaction Pool related semaphores
  sem_t *tx_pool_mutex = sem_open("TX_POOL_MUTEX", 0);
  if (tx_pool_mutex == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m tx_pool_mutex not initialized yet. The Controller process has not been launched. Closing.\n");
    exit(-1);
  }
  sem_t *tx_pool_full = sem_open("TX_POOL_FULL", 0);
  if (tx_pool_full == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m tx_pool_full not initialized yet. The Controller process has not been launched. Closing.\n");
    exit(-1);
  }
  sem_t *tx_pool_empty = sem_open("TX_POOL_EMPTY", 0);
  if (tx_pool_empty == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m tx_pool_empty not initialized yet. The Controller process has not been launched. Closing.\n");
    exit(-1);
  }


  // Transaction Pool 
  // -- Access the already created shared memory
  key_t tx_key = ftok("config.cfg", 'K');
  int tx_pool_id;
  if ((tx_pool_id = shmget(tx_key, 0, 0766)) < 0) {
    printf("[TxGen] [PID %d] Error accessing the Transaction Pool (Shared Memory)\n", getpid());
    exit(-1);
  }
  if (DEBUG)
    printf("[TxGen] [PID %d] Successfully accessed the Transaction Pool (Shared Memory)\n", getpid());

  // -- Attach to the shared memory
  TxPoolNode *tx_pool;
  if ((tx_pool = (TxPoolNode*)shmat(tx_pool_id, NULL, 0)) < 0) {
    printf("[TxGen] [PID %d] Error attaching to the Transaction Pool (Shared Memory)\n", getpid());
    exit(-1);
	}
  if (DEBUG) {
    printf("[TxGen] [PID %d] Successfully attached to the Transaction Pool\n", getpid());
    printf("[TxGen] shmget key: %d | shmid: %d\n", tx_key, tx_pool_id);
  }


  // -- Get the size of Transaction Pool
  struct shmid_ds buf;
  shmctl(tx_pool_id, IPC_STAT, &buf);
  int tx_pool_size = (int)buf.shm_segsz / sizeof(TxPoolNode);
  printf("[TxGen] [PID %d] tx_pool_size = %d\n", getpid(), tx_pool_size);

  int increment = 1;
  while (1) {
    // Generate a transaction
    printf("\n[Tx Gen] [PID %d] ===== NEW TRANSACTION =====\n", getpid());
    char id[64];
    sprintf(id, "TX-%d-%d", getpid(), increment++);
    printf("[Tx Gen] [PID %d] Transaction ID = %s\n", getpid(), id);
    printf("[Tx Gen] [PID %d] Reward = %d\n", getpid(), reward);
    int value = rand() % 100 + 1;
    printf("[Tx Gen] [PID %d] Value = %d\n", getpid(), value);
    Timestamp current_time = get_timestamp();
    printf("[Tx Gen] [PID %d] Timestamp = %02d:%02d:%02d\n", getpid(), current_time.hour, current_time.min, current_time.sec);

    // Write the transaction in shared memory
    // -- Find the first empty slot in the Transaction Pool
    sem_wait(tx_pool_empty);
    sem_wait(tx_pool_mutex);
    if (DEBUG)
      printf("[Tx Gen] [PID %d] Writing the transaction to the Transaction Pool...\n", getpid());
    int i = 0;
    while (tx_pool[i].empty != 1) {
      i = (i + 1) % tx_pool_size;
    }
    strcpy(tx_pool[i].tx.id, id);
    tx_pool[i].tx.reward = reward;
    tx_pool[i].tx.value = value;
    tx_pool[i].tx.timestamp = current_time;
    tx_pool[i].empty = 0;
    sem_post(tx_pool_mutex);
    sem_post(tx_pool_full);
    if (DEBUG)
      printf("[Tx Gen] [PID %d] Transaction successfully written to the Transaction Pool.\n", getpid());

    sleep(2); // -- TODO: revert the sleep time back to 'sleeptime'
  }

  // Process termination
  sprintf(msg, "[Tx Gen] [PID %d] Process terminated", getpid());
  log_message(msg, 'r', DEBUG);
  return 0;
}
