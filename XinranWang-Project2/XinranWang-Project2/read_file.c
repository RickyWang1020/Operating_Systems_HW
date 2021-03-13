#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "read_file.h"
#include "sorter.h"

/* helps to allocate memory for a string and puts the string into an array of string */
void alloc_mem_for_str(char** arr, int position, char* txt, int mem_size){
    
    /* create a pointer of string */
    char *ptr = (char*) malloc(mem_size * sizeof(char*));
    /* copy the content to the string pointer */
    strcpy(ptr, txt);
    //* put the string pointer in the wanted position in the string array */
    arr[position] = ptr;
}

/* a helper function to get the no. of lines in the file */
int get_no_of_lines(char* filename){
    FILE *file;
    char line[2000];
    file = fopen(filename, "r");
    int line_count = 0;

    /* loop through every line and increment the line no. counter */
    while (fgets(line, sizeof(line), file)){
        line_count ++;
    }
    return line_count;
}

/* parse out every line of data from the given file, and store each line into an entry of string array */
void separate_line(char *filename, char *output_arr[], int line_no){
    FILE *file;
    file = fopen(filename, "r");

    /* loop through every line */
    char line[500];
    for (int c = 0; c < line_no; c++){
        int count;
        size_t length;
        /* get a line of data */
        fgets(line, sizeof(line), file);
        length = strlen(line);
        /* this helps to remove the new-line character at the end of each data line */
        if (line[length - 1] == '\n')
        line[--length] = '\0';

        /* allocate memory for this line of data and put it into the corresponding entry of a string array */
        char *ptr = (char*) malloc(500 * sizeof(char*));
        strcpy(ptr, line);
        output_arr[c] = ptr;
    }
}

/* for every line of data, split it by spaces and get the attribute data
we want to perform sort on, and store this numeric data into a double-type array */
void parse_wanted_num(char *data_line_arr[], double *output_arr, int attribute_no, int list_length){
    
    /* loop through every line of data */
    for (int count = 0; count < list_length; count++){
        char* line = (char*) malloc(1000 * sizeof(char*));
        strcpy(line, data_line_arr[count]);

        /* parse the line by white space */
        const char *delimiters = " \t";
        
        /* now we want to get the specific attribute field's numeric data */
        char *element = strtok(line, delimiters);
        int current = 0;

        while (current != attribute_no){
            element = strtok(NULL, delimiters);
            current ++;
        }

        /* put the parsed numeric data into the corresponding position in the numeric array */
        output_arr[count] = atof(element);
        free(line);
    }
}
