/*
  DEIChain - Utilities Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)

  This file contains some utility functions used to perform certain
  actions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <semaphore.h>

#include "utils.h"
#include "structs.h"

#define BUFFER_SIZE 100

extern FILE *log_file;

/*
  Function to log a message to the log file and, optionally, on the console
*/
void log_message(char *msg, char msg_type, int verbose) {
  // Open the named semaphore
  sem_t *log_mutex = sem_open("LOG_MUTEX", 0);
  if (log_mutex == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m log_mutex not initialized yet. The Controller process has not been launched. Closing.\n");
    exit(-1);
  }

  // Get the current date and time
  time_t current_time;
  time(&current_time);
  struct tm *local = localtime(&current_time);
  
  int hours = local->tm_hour;
  int minutes = local->tm_min;
  int seconds = local->tm_sec;

  int day = local->tm_mday;
  int month = local->tm_mon + 1;    // month (goes from 0 to 11)
  int year = local->tm_year + 1900;  // year since 1900

  // Log the message to the log file
  sem_wait(log_mutex);
  
  // -- Write the log message to the log file
  fprintf(log_file, "[%02d/%02d/%d - %02d:%02d:%02d] %s\n", day, month, year, hours, minutes, seconds, msg);
  fflush(log_file);
  sem_post(log_mutex);

  // -- Print the message on the screen
  if (verbose == 1 && msg_type == 'r')
    printf("\x1b[33m[*]\x1b[0m %s\n", msg);
  if (verbose == 1 && msg_type == 'w')
    printf("\x1b[31m[!]\x1b[0m %s\n", msg);
}


/*
  Initializes the variables passed as arguments with the values from the
  configuration file 'config.cfg'
*/
void load_config(int *num_miners, int *tx_pool_size, int *transactions_per_block, int *blockchain_blocks) {
  // Open the file
  char buffer[BUFFER_SIZE];
  FILE *config_file;
  if ((config_file = fopen("config.cfg", "r")) == NULL) {
    log_message("Could not load the configuration file", 'w', 1);
    exit(-1);
  }

  // Parse each line of the configuration file
  int line = 0;
  while (line < 4) {
    if (fgets(buffer, BUFFER_SIZE, config_file) == NULL) {
      log_message("Invalid configuration file", 'w', 1);
      exit(-1);
    }

    buffer[strcspn(buffer, "\n")] = '\0';   // Remove the '\n' character
    
    // Assign the read value to the correct variable
    if (line == 0 && (*num_miners = convert_to_int(buffer)) == 0) {
      log_message("Invalid value for NUM_MINERS", 'w', 1);
      exit(-1);
    } else if (line == 1 && (*tx_pool_size = convert_to_int(buffer)) == 0) {
      log_message("Invalid value for TX_POOL_SIZE", 'w', 1);
      exit(-1);
    } else if (line == 2 && (*transactions_per_block = convert_to_int(buffer)) == 0) {
      log_message("Invalid value for TRANSACTIONS_PER_BLOCK", 'w', 1);
      exit(-1);
    } else if (line == 3 && (*blockchain_blocks = convert_to_int(buffer)) == 0) {
      log_message("Invalid value for BLOCKCHAIN_BLOCKS", 'w', 1);
      exit(-1);
    }
    line++;
  }

  fclose(config_file);
}


/*
  Auxiliary function to convert a number in the string format to an integer
*/
int convert_to_int(char *str) {
  int res = 0;
  int len = strlen(str);
  int mult = 1;
  for (int i = len-1; i >= 0; i--) {
    int cur_digit = str[i] - '0';
    if (cur_digit < 0 || cur_digit > 9)
      return 0;
    else {
      res += cur_digit * mult;
      mult *= 10;
    }
  }
  return res;
}


/*
  Auxiliary function to generate a timestamp
*/
Timestamp get_timestamp() {
  Timestamp res;

  // Get the current date and time
  time_t current_time;
  time(&current_time);
  struct tm *local = localtime(&current_time);
  
  res.hour = local->tm_hour;
  res.min = local->tm_min;
  res.sec = local->tm_sec;

  return res;
}


/*
  Auxiliary function to get the memory addresses of the blocks and the
  respective transactions in shared memory
*/
void get_blockchain_mapping(TxBlock *blockchain_ledger, int num_blocks, int tx_per_block, TxBlock **blocks) {
  // Cursor with the shared memory's address
  char *cursor = (char*)blockchain_ledger;
  
  // Assign the blocks array
  *blocks = (TxBlock*)cursor;
  cursor += sizeof(TxBlock) * num_blocks;

  // Assign the transactions array
  Tx *transactions = (Tx*)cursor;

  // Map each block's transactions array
  for (int i = 0; i < num_blocks; i++)
    (*blocks)[i].transactions = transactions + i * (sizeof(Tx) * tx_per_block);
}


/*
  Auxiliar function to dump the data from the Blockchain Ledger
*/
void dump_ledger(TxBlock *blocks, int num_blocks, int tx_per_block) {
  sem_t *ledger_mutex = sem_open("LEDGER_MUTEX", 0);
  sem_t *log_mutex = sem_open("LOG_MUTEX", 0);
  if (ledger_mutex == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m ledger_mutex not initialized yet. Closing.\n");
    exit(-1);
  }
  if (log_mutex == SEM_FAILED) {
    printf("\x1b[31m[!]\x1b[0m log_mutex not initialized yet. Closing.\n");
    exit(-1);
  }

  // Open the log file
  sem_wait(ledger_mutex);
  sem_wait(log_mutex);

  // Create dummy blocks for testing purposes
  srand(time(NULL));
  for (int x = 0; x < 10; x++) {
    TxBlock *block = &blocks[x];
    char temp[64];
    sprintf(temp, "BLOCK-%d-%d", rand() % 1000 + 1000, rand() % 100 + 1);
    strcpy(block->id, temp);
    strcpy(block->previous_block_hash, "00006a8e76f31ba74e21a092cca1015a418c9d5f4375e7a4fec676e1d2ec1436");
    block->timestamp = get_timestamp();
    block->nonce = rand() % 1680540;
    for (int i = 0; i < 3; i++) {
      sprintf(temp, "TX-%d-%d", rand() % 1000 + 1000, rand() % 100 + 1);
      strcpy(block->transactions[i].id, temp);
      block->transactions[i].reward = rand() % 2 + 1;
      block->transactions[i].value = (double)(rand() % 100 + 1);
      block->transactions[i].timestamp = get_timestamp();
    }
  }
  
  FILE *log_file = fopen("DEIChain_log.txt", "a");
  if (log_file == NULL) {
    printf("\x1b[31m[!]\x1b[0m [Controller] Error opening the log file\n");
    exit(-1);
  }

  // Get the data from the blockchain ledger
  char buffer[2000];
  fprintf(log_file, "[Controller] Dumping the Blockchain Ledger");
  for (int i = 0; i < num_blocks; i++) {
    TxBlock *block = &blocks[i];
    if (block->timestamp.hour == 0 && block->timestamp.min == 0 && block->timestamp.sec == 0)
      continue;
    snprintf(buffer, sizeof(buffer),
        "\n┌────────────────────────────────────────────────────────────────────────┐\n"
        "│                            Block %-4d                                  │\n"
        "├────────────────────────────────────────────────────────────────────────┤\n"
        "│ Block ID: %-20s                                         │\n"
        "│ Previous Hash:                                                         │\n"
        "│   %-69s│\n"
        "│ Block Timestamp: %02d:%02d:%02d                                              │\n"
        "│ Nonce: %-10d                                                      │\n"
        "├────────────────────────────────────────────────────────────────────────┤\n"
        "│                          Transactions                                  │\n"
        "├────────────────────┬──────────────┬───────────────┬────────────────────┤\n"
        "│ TX ID              │ Reward       │ Value         │ Timestamp          │\n"
        "├────────────────────┼──────────────┼───────────────┼────────────────────┤\n",
        i, block->id, block->previous_block_hash, block->timestamp.hour, block->timestamp.min,
        block->timestamp.sec, block->nonce
    );
    fprintf(log_file, buffer);
    printf(buffer);
    // Print Transactions for the Block
    for (int j = 0; j < tx_per_block; j++) {
      Tx tx = block->transactions[j];
      if (tx.reward > 0 && tx.reward < 4) {
        sprintf(buffer, "│ %-11s        │ %-1d            │ %6.2lf        │ %02d:%02d:%02d           │\n",
          tx.id, tx.reward, tx.value, tx.timestamp.hour, tx.timestamp.min, tx.timestamp.sec);
        fprintf(log_file, buffer);
        printf(buffer);
      }
    }
    sprintf(buffer, "└────────────────────┴──────────────┴───────────────┴────────────────────┘\n");
    fprintf(log_file, buffer);
    printf(buffer);
  }
  fprintf(log_file, "[Controller] Blockchain Ledger dumped successfully");
  fclose(log_file);
  sem_post(log_mutex);
  sem_post(ledger_mutex);
}