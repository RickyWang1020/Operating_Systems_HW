#ifndef NODE_H
#define NODE_H

extern int no_of_sig2;

void get_sort_range(int* start_from, int* sort_ranges, int no_of_worker, int random_range, int no_of_lines);
void create_coord(char* file_address, int no_of_worker, int random_range, int attribute_number, int order, char* output_address, int no_of_lines, pid_t root_id);
void merge_data(int *merged_index, double *number_data, int **index_lists, int no_of_worker, int no_of_lines, int *sort_ranges, int order);
void sig2_handler(int signum);

#endif
