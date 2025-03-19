#ifndef HEADER_H
#define HEADER_H

// -- Controller
void erro(char*);
void load_config(int*, int*, int*, int*, int*);
void cleanup();

// -- Miner
void* miner_routine(void*);
void miner(int);

// -- Validator
void validator();

// -- Statistics
void statistics();

#endif