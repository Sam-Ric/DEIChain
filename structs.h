#ifndef STRUCTS_H
#define STRUCTS_H

/*
  Timestamp structure
*/
typedef struct {
  int day;
  int month;
  int year;
  int hour;
  int min;
  int sec;
} Timestamp;

/*
  Block structure
*/
typedef struct {

} Block;

/*
  Blockchain Ledger structure
*/
typedef struct {
  int dummy_argument;
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
} Transaction;

/*
  Transaction Pool structure
*/
typedef struct {
  int current_block_id;
  Transaction *slots;
} TransactionPool;

#endif