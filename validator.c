/*
  DEIChain - Validator
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "validator.h"

#define DEBUG 1

void validator() {
  char msg[100];
  sprintf(msg, "[Validator] Process initialized (parent PID -> %d)", getppid());
  log_message(msg, 'r', DEBUG);

}