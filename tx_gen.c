/*
  DEIChain - Transaction Generator Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

#define DEBUG 1

int main(int argc, char *argv[]) {
  // Verify the given arguments
  if (argc != 3) {
    log_message("[Tx Gen] Error creating a Transaction Generator", 'w', 1);
    printf("Correct format: TxGen <reward> <sleeptime>");
    exit(-1);
  }

  // Assign the arguments to the respective variables
  int reward;
  int sleeptime;
  if ((reward = convert_to_int(argv[1])) == 0 || reward < 1 || reward > 3) {
    log_message("[Tx Gen] Invalid reward parameter", 'w', 1);
    printf("reward: 1 to 3\n");
    exit(-1);
  }
  if ((sleeptime = convert_to_int(argv[2])) == 0 || sleeptime < 200 || sleeptime > 3000) {
    log_message("[Tx Gen] Invalid sleep time parameter", 'w', 1);
    printf("sleeptime (ms): 200 to 3000\n");
    exit(-1);
  }

  // Process initialization
  char msg[100];
  sprintf(msg, "[Tx Gen] [PID %d] Process initialized", getpid());
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Tx Gen] [PID %d] reward = %d", getpid(), reward);
  log_message(msg, 'r', DEBUG);
  sprintf(msg, "[Tx Gen] [PID %d] sleeptime = %d", getpid(), sleeptime);
  log_message(msg, 'r', DEBUG);

  /* ---- Transaction Generator Code ---- */

  // Process termination
  sprintf(msg, "[Tx Gen] [PID %d] Process terminated", getpid());
  log_message(msg, 'r', DEBUG);
  return 0;
}
