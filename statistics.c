/*
  DEIChain - Statistics
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "statistics.h"

#define DEBUG 1

void statistics() {
  char msg[100];
  sprintf(msg, "[Statistics] Process initialized (parent PID -> %d)", getppid());
  log_message(msg, 'r', DEBUG);
  
}
