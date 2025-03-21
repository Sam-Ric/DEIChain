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

#define BUFFER_SIZE 100


/*
  Perform the cleanup of the IPC mechanisms
*/
void cleanup() {

}

/*
  Function to log a message to the log file and, optionally, on the console
*/
void log_message(char *msg, char msg_type, int verbose) {
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

  // Open the log file for writing
  FILE *log_file = fopen("DEIChain_log.cfg", "a");
  
  // Write the log message to the log file
  fprintf(log_file, "[%02d/%02d/%d - %02d:%02d:%02d] %s\n", day, month, year, hours, minutes, seconds, msg);
  fclose(log_file);

  // Print the message on the screen
  if (verbose == 1 && msg_type == 'r')
    printf("\x1b[33m[*]\x1b[0m %s\n", msg);
  if (verbose == 1 && msg_type == 'w')
    printf("\x1b[31m[!]\x1b[0m %s\n", msg);
}


/*
  Initializes the variables passed as arguments with the values from the
  configuration file 'config.cfg'
*/
void load_config(int *num_miners, int *pool_size, int *transactions_per_block, int *blockchain_blocks, int *transaction_pool_size) {
  // Open the file
  char buffer[BUFFER_SIZE];
  FILE *config_file;
  if ((config_file = fopen("config.cfg", "r")) == NULL) {
    log_message("Could not load the configuration file", 'w', 1);
    exit(-1);
  }

  int line = 0;
  while (line < 4) {
    if (fgets(buffer, BUFFER_SIZE, config_file) == NULL) {
      log_message("Invalid configuration file", 'w', 1);
      exit(-1);
    }

    buffer[strcspn(buffer, "\n")] = '\0';   // Remove the '\n' character
    
    // Assign the read value to the correct variable
    if (line == 0 && (*num_miners = atoi(buffer)) == 0) {
      log_message("Invalid value for NUM_MINERS", 'w', 1);
      exit(-1);
    } else if (line == 1 && (*pool_size = atoi(buffer)) == 0) {
      log_message("Invalid value for POOL_SIZE", 'w', 1);
      exit(-1);
    } else if (line == 2 && (*transactions_per_block = atoi(buffer)) == 0) {
      log_message("Invalid value for TRANSACTIONS_PER_BLOCK", 'w', 1);
      exit(-1);
    } else if (line == 3 && (*blockchain_blocks = atoi(buffer)) == 0) {
      log_message("Invalid value for BLOCKCHAIN_BLOCKS", 'w', 1);
      exit(-1);
    }
    line++;
  }

  // Assign a value to the TRANSACTION_POOL_SIZE variable
  // -- If there is a custom value in the file, assign that value
  if (fgets(buffer, BUFFER_SIZE, config_file) != NULL) {
    if ((*transaction_pool_size = atoi(buffer)) == 0) {
      log_message("Invalid value for TRANSACTION_POOL_SIZE", 'w', 1);
      exit(-1);
    }
  }
  // Else, use the default value of 10000
  else
    *transaction_pool_size = 10000;

  fclose(config_file);
}
