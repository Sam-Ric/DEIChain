/*
  DEIChain - Structs Header File
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#ifndef STRUCTS_H
#define STRUCTS_H

#define DEBUG 1
#define TXB_ID_LEN 64
#define PIPE_NAME "/tmp/VALIDATOR_INPUT"
#define HASH_SIZE 65

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
  char id[64];
  int reward;
  double value;
  Timestamp timestamp;
} Tx;

/*
  Block structure
*/
typedef struct {
  char id[TXB_ID_LEN];
  char previous_block_hash[HASH_SIZE];
  Timestamp timestamp;
  Tx *transactions;
  int nonce;
} TxBlock;

/*
  Transaction Pool Node structure
*/
typedef struct {
  int empty;
  int age;
  Tx tx;
  int selected;
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

// Miner Shared Memory data
struct MinerArgs {
  int num_miners;
  TxPoolNode *tx_pool;
  int tx_pool_size;
  int tx_per_block;
  TxBlock *blocks;
};

// Message Queue message format
typedef struct {
  long msgtype;
  int valid_block;
  int miner_id;
  int credits;
  Timestamp creation_time;
  Timestamp validation_time;
} Message;

typedef struct {
  int miner_id;
  char result_hash[HASH_SIZE];
  TxBlock block;
  Tx transactions[];
} PipeMsg;

#endif