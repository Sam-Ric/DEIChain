/*
  DEIChain - Validator
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023206471)
*/

#include <stdio.h>
#include <unistd.h>

#define DEBUG

void validator() {

  #ifdef DEBUG
    printf("[DEBUG] [Validator] Process initialized (parent PID -> %d)\n", getppid());
  #endif

}