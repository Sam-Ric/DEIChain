/*
  DEIChain - Statistics Header File
  by
    Samuel Riça (2023206471)
    Diogo Santos (2023211097)
*/

#ifndef STATISTICS_H
#define STATISTICS_H

void statistics();

/*
  Prints the simulation statistics
*/
void print_statistics();

/*
  Calculate the number of seconds between two timestamps
*/
double calc_timestamp_difference(Timestamp t1, Timestamp t2);

#endif
