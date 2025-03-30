/*
  DEIChain - Utilities Header File
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)

  This file contains some utility functions used to perform certain
  actions.
*/

#ifndef UTILS_H
#define UTILS_H

/*
  Function to log a message to the log file and to the console, if the verbose
  option is enabled
*/
void log_message(char* message, char msg_type, int verbose);

/*
  Function to load the data written on the configuration file and initialize
  the required variables
*/
void load_config(int *num_miners, int *tx_pool_size, int *transactions_per_block, int *blockchain_blocks);

/*
  Auxiliary function to convert a number written as a string to an integer
*/
int convert_to_int(char *str);

#endif
