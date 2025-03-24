/*
  DEIChain - Transaction Generator
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#include <stdio.h>
#include <stdlib.h>

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
  if ((reward = atoi(argv[1])) == 0 || reward < 1 || reward > 3) {
    log_message("[Tx Gen] Invalid reward parameter", 'w', 1);
    printf("reward: 1 to 3\n");
    exit(-1);
  }
  if ((sleeptime = atoi(argv[2])) == 0 || sleeptime < 200 || sleeptime > 3000) {
    log_message("[Tx Gen] Invalid sleep time parameter", 'w', 1);
    printf("sleeptime (ms): 200 to 3000\n");
    exit(-1);
  }

  // If the given arguments are within the standards
  log_message("[Tx Gen] Transaction Generator created successfully", 'r', DEBUG);
  return 0;
}
