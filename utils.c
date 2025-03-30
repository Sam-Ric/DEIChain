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

#define BUFFER_SIZE 100

/*
  Function to log a message to the log file and, optionally, on the console
*/
void log_message(char *msg, char msg_type, int verbose) {
  // Open the named semaphore
  sem_t *log_mutex = sem_open("LOG_MUTEX", 0);

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

  // -- Open the log file for writing
  FILE *log_file = fopen("DEIChain_log.txt", "a");
  if (log_file == NULL) {
    printf("\x1b[31m[!]\x1b[0m [Controller] Error opening the log file\n");
    exit(-1);
  }
  
  // -- Write the log message to the log file
  fprintf(log_file, "[%02d/%02d/%d - %02d:%02d:%02d] %s\n", day, month, year, hours, minutes, seconds, msg);
  fflush(log_file);
  fclose(log_file);
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
