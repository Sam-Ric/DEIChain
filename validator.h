/*
  DEIChain - Validator Header File
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "structs.h"

void validator();

/*
  Auxiliary function to copy a Block from SRC to DEST, being DEST an
  address in the Blockchain Ledger
*/
int save_block(TxBlock **ledger, TxBlock *src, int num_blocks, int tx_per_block);

#endif
