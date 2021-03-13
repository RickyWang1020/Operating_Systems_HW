#ifndef READ_FILE_H
#define READ_FILE_H
#include "sorter.h"

void alloc_mem_for_str(char** arr, int position, char* txt, int mem_size);
int get_no_of_lines(char* filename);
void separate_line(char *filename, char *output_arr[], int line_no);
void parse_wanted_num(char *data_line_arr[], double *output_arr, int attribute_no, int list_length);

#endif