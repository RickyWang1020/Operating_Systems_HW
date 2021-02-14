#ifndef READ_DATA_H
#define READ_DATA_H

extern int table_size;

int split (const char *txt, char delim, char ***tokens);
Voter *read_csv(char *filename, Voter *data);
struct Hash_and_ll bulk_vote(Voter *table, Postcode *post_ll, char* address);

#endif