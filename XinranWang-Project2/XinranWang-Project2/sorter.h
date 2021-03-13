#ifndef SORTER_H
#define SORTER_H

extern int no_of_sig1;

struct pollfd;

void prepare_and_sort(double *all_data_for_read, int sorter_idx, int start, int range, int order, int root_id);
void init_fd_for_worker(struct pollfd *pollfds, int worker);
void sig1_handler(int signum);

#endif