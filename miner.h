/*
  DEIChain - Miner Header File
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#ifndef MINER_H
#define MINER_H

#include "structs.h"

void* miner_routine(void*);
void miner(int, struct MinerArgs);

#endif
