/*
  DEIChain - Validator Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "utils.h"
#include "validator.h"

#define DEBUG 1

void validator() {
  // Process initialization
  char msg[100];
  sprintf(msg, "[Validator] Process initialized (PID -> %d | parent PID -> %d)", getpid(), getppid());
  log_message(msg, 'r', DEBUG);

  /* ---- Validator Code ---- */
  while (1) {
    printf("[Validator] Running...\n");
    sleep(2);
  }

  // Process termination
  log_message("[Validator] Process terminated", 'r', 1);
}

/*
  Auxiliary function to copy a Block from the SRC address to an
  address in the Blockchain Ledger
*/
int save_block(TxBlock **ledger, TxBlock *src, int num_blocks, int tx_per_block) {
  // Iterate over the Blockchain Ledger and check for the first available slot
  TxBlock *block;
  for (int i = 0; i < num_blocks; i++) {
    block = ledger[i];
    if (block->id[0] != '\0') { // -> Checks if the block is already occupied
      // Place the SRC Block data in the Blockchain Ledger
      strcpy(block->id, src->id);
      strcpy(block->previous_block_hash, src->previous_block_hash);
      block->timestamp = src->timestamp;
      block->nonce = src->nonce;
      for (int j = 0; j < tx_per_block; j++) {
        strcpy(block->transactions[j].id, src->transactions[j].id);
        block->transactions[j].reward = src->transactions[j].reward;
        block->transactions[j].timestamp = src->transactions[j].timestamp;
        block->transactions[j].value = src->transactions[j].value;
      }
      return 0;
    }
  }
  return -1;
}