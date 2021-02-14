#ifndef HASH_H
#define HASH_H

#include "voter.h"

#define h(x, m) (x % m) // hash function is now: x mod m

extern int people_in_hash;
extern int table_size;

Voter *init_hash_table(Voter *table);
void print_hash_table(Voter *table);
Voter *insert(Voter *table, char *first, char *last, int rin, int zip);
void search(Voter *table, int rin);
Voter *free_hash_table_element(Voter *table);

#endif