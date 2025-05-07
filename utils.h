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

#include "structs.h"

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

/*
  Auxiliary function to generate a timestamp
*/
Timestamp get_timestamp();

/*
  Auxiliary function to get the memory addresses of the blocks and the
  respective transactions in shared memory
*/
void get_blockchain_mapping(TxBlock *blockchain_ledger, int num_blocks, int tx_per_block, TxBlock **blocks, char **last_hash);

/*
  Auxiliar function to dump the data from the Blockchain Ledger
*/
void dump_ledger(TxBlock *blocks, int num_blocks, int tx_per_block);

/*
  Prints a block's data
*/
void print_block(TxBlock block, int tx_per_block);

/*
  Auxiliary function that implements the aging mechanism of the Transactions Pool
*/
void increment_age(TxPoolNode *tx_pool, int size);

#endif
