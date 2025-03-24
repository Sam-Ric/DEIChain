#ifndef UTILS_H
#define UTILS_H

void cleanup();
void log_message(char* message, char msg_type, int verbose);
void load_config(int *num_miners, int *pool_size, int *transactions_per_block, int *blockchain_blocks, int *transaction_pool_size);
int convert_to_int(char *str);

#endif
