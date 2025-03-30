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
  Block structure
*/
typedef struct {
  int dummy_attribute;
} Block;

/*
  Blockchain Ledger structure
*/
typedef struct {
  int dummy_attribute;
} BlockchainLedger;

/*
  Transaction structure
*/
typedef struct {
  int id;
  int reward;
  int sender_id;
  int receiver_id;
  int value;
  Timestamp timestamp;
} Tx;


/*
  Transaction Pool Node structure
*/
typedef struct {
  int empty;
  int age;
  Tx *transaction;
} TxPoolNode;

/*
  Transaction Pool structure
*/
typedef struct {
  int current_block_id;
  TxPoolNode *transactions_pending_set;
} TxPool;

#endif