/*
  DEIChain - Validator Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
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

  // Ignore ^C signal
  signal(SIGINT, SIG_IGN);
  // Ignore ^T signal
  signal(SIGUSR1, SIG_IGN);

  /* ---- Validator Code ---- */
  while (1) {
    printf("[Validator] Running...\n");
    sleep(2);
  }

  // Process termination
  log_message("[Validator] Process terminated", 'r', 1);
}
