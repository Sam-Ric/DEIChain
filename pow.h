/* pow.h */
#ifndef POW_H
#define POW_H

#include <stdio.h>
#include <stdlib.h>

#include "structs.h"

#define POW_MAX_OPS 10000000

#define INITIAL_HASH \
  "00006a8e76f31ba74e21a092cca1015a418c9d5f4375e7a4fec676e1d2ec1436"

extern int tx_per_block;

/* Definition of Difficulty Levels */
typedef enum { EASY = 1, NORMAL = 2, HARD = 3 } DifficultyLevel;

typedef struct {
  char hash[HASH_SIZE];
  double elapsed_time;
  int operations;
  int error;
} PoWResult;

int get_max_transaction_reward(const TxBlock *block, const int txs_per_block);
void compute_sha256(const TxBlock *input, char *output);
PoWResult proof_of_work(TxBlock *block);
int verify_nonce(const TxBlock *block);
int check_difficulty(const char *hash, const int reward);
DifficultyLevel getDifficultFromReward(const int reward);

// Inline function to compute the size of a TransactionBlock
static inline size_t get_transaction_block_size() {
  if (tx_per_block == 0) {
    perror("Must set the 'transactions_per_block' variable before using!\n");
    exit(-1);
  }
  return sizeof(TxBlock) + tx_per_block * sizeof(Tx);
}

#endif
/* POW_H */