/*
  DEIChain - Statistics
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <unistd.h>

#define DEBUG

void statistics() {

  #ifdef DEBUG
    printf("[DEBUG] [Statistics] Process initialized (parent PID -> %d)\n", getppid());
  #endif
  
}
