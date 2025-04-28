/*
  DEIChain - Structs Header File
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#ifndef STRUCTS_H
#define STRUCTS_H

/*
  Timestamp structure
*/
typedef struct {
  int hour;
  int min;
  int sec;
} Timestamp;

/*
  Transaction structure
*/
typedef struct {
  int id;
  int reward;
  double value;
  Timestamp timestamp;
} Tx;

/*
  Block structure
*/
typedef struct {
  int id;
  char previous_block_hash[100];
  Timestamp timestamp;
  int nonce;
} TxBlock;

/*
  Transaction Pool Node structure
*/
typedef struct {
  int empty;
  int age;
  Tx tx;
} TxPoolNode;

/*
  PoW structure
*/
typedef struct {
  char hashing_algorithm[10];
  char algorithm[10];
  int reward;
  int max_operations;
} PoW;

// Validator Manager's variables
struct ValidatorThreadArgs {
  TxPoolNode *tx_pool;
  int tx_pool_size;
};

// Miner Shared Memory data
struct MinerArgs {
  int num_miners;
  TxPoolNode *tx_pool;
  int tx_pool_size;
  int tx_per_block;
  TxBlock *blocks;
  Tx *transactions;
};

// Miner thread arguments
struct MinerThreadArgs {
  int miner_id;
  int tx_per_block;
};

#endif