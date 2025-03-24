/*
  DEIChain - Statistics Source Code
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
  // Process initialization
  char msg[100];
  sprintf(msg, "[Statistics] Process initialized (parent PID -> %d)", getppid());
  log_message(msg, 'r', DEBUG);

  /* ---- Statistics Code ---- */
  
  // Process termination
  log_message("[Statistics] Process terminated", 'r', 1);
}
