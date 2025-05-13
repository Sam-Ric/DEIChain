/*
  DEIChain - Statistics Source Code
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#define _POSIX_C_SOURCE 200809L // Signal library MACRO fix

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <errno.h>

#include "utils.h"
#include "statistics.h"
#include "structs.h"

extern FILE *log_file;

extern sem_t *stats_done;

extern int msq_id;
extern pid_t controller_pid;
extern int num_miners;
extern int blockchain_blocks;

int stats_in_progress = 0;

// Statistic Metrics
int *valid_blocks_per_miner;   // Number of valid blocks submitterd by each Miner to the Validator
int *invalid_block_per_miner;  // Number of invalid blocks submitted by each Miner to the Validator
double avg_time;                // Average time to verify a transaction
int *credits_per_miner;        // Credits of each Miner
int total_block_count;          // Total number of blocks validated (correct/incorrect)
int blockchain_count;           // Total number of blocks in the Blockchain

double total_verification_time; // Cumulative verification time

void statistics() {
  // Process initialization
  char msg[100];
  sprintf(msg, "[Statistics] Process initialized (PID -> %d | parent PID -> %d)", getpid(), getppid());
  log_message(msg, 'r', DEBUG);

  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_handler = print_statistics;
  sigaction(SIGUSR1, &act, NULL);

  // Variable initialization
  total_verification_time = 0.0;
  valid_blocks_per_miner = calloc(num_miners, sizeof(int));
  invalid_block_per_miner = calloc(num_miners, sizeof(int));
  credits_per_miner = calloc(num_miners, sizeof(int));
  avg_time = 0.0;
  total_block_count = 0;
  blockchain_count = 0;

  Message recv;     // -> Buffer to store the messages received via message queue

  while (1) {
    // Process waits for the semaphore to be released to read from the message queue
    if (DEBUG)
      printf("    [Statistics] Process on hold\n");
    // Read from the message queue (while no messages => blocked state)
    if (msgrcv(msq_id, &recv, sizeof(Message) - sizeof(long), 0, 0) < 0) {
      if (errno == EINTR) continue;
      else
        log_message("[Statistics] msgrcv error", 'w', 1);
    }
    if (DEBUG)
      printf("    [Statistics] Calculating statistics...\n");
    // -- Update the variables
    total_block_count++;
    int miner_index = recv.miner_id - 1;
    if (recv.valid_block) {
      blockchain_count++;
      valid_blocks_per_miner[miner_index]++;
      total_verification_time += calc_timestamp_difference(recv.creation_time, recv.validation_time);
      avg_time = (double)(total_verification_time / blockchain_count);
      credits_per_miner[miner_index] += recv.credits;
    } else
      invalid_block_per_miner[miner_index]++;
    if (blockchain_count == blockchain_blocks) {
      log_message("[Statistics] Blockchain Ledger is full. Closing...", 'r', 1);
      kill(controller_pid, SIGINT);
    }
    if (DEBUG)
      printf("    [Statistics] Statistics calculated succesfully\n");
  }

  // Process termination
  log_message("[Statistics] Process terminated", 'r', 1);
}

/*
  Prints the simulation statistics
*/
void print_statistics(int signum) {
  if (stats_in_progress)
    return;
  stats_in_progress = 1;
  log_message("[Statistics] Printing statistics...", 'r', 1);
  char buffer[2000];
  snprintf(buffer, sizeof(buffer),
      "\n┌────────────────────────────────────────────────────────────────────────┐\n"
      "│                             Statistics                                 │\n"
      "├────────────────────────────────────────────────────────────────────────┤\n"
      "│ Total Block Count: %-10d                                          │\n"
      "│ Blocks in the Blockchain: %-10d                                   │\n"
      "│ Average Time to Verify: %10.2f seconds                             │\n"
      "├────────────────────────────────────────────────────────────────────────┤\n"
      "│                          Miner Performance                             │\n"
      "├────────────┬───────────────┬────────────────┬──────────────────────────┤\n"
      "│ Miner ID   │ Valid Blocks  │ Invalid Blocks │ Total Credits            │\n"
      "├────────────┼───────────────┼────────────────┼──────────────────────────┤\n",
      total_block_count, blockchain_count, avg_time
  );
  fprintf(log_file, buffer);
  printf(buffer);
  
  char row[100];
  for (int i = 0; i < num_miners; i++) {
    snprintf(row, sizeof(row),
        "│ %-10d │ %-13d │ %-14d │ %-21d    │\n",
        i+1, valid_blocks_per_miner[i], invalid_block_per_miner[i], credits_per_miner[i]);
    fprintf(log_file, row);
    printf(row);
  }

  sprintf(buffer, "└────────────┴───────────────┴────────────────┴──────────────────────────┘\n");
  fprintf(log_file, buffer);
  fflush(log_file);
  printf(buffer);
  sem_post(stats_done);
  stats_in_progress = 0;
}

/*
  Calculate the number of seconds between two timestamps (t1 - t2)
*/
double calc_timestamp_difference(Timestamp t1, Timestamp t2) {
  int t1_seconds = t1.hour * 3600 + t1.min * 60 + t1.sec;
  int t2_seconds = t2.hour * 3600 + t2.min * 60 + t2.sec;
  return (double)(t2_seconds - t1_seconds);
}
