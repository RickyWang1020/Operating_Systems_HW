#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <sys/times.h>
#include "node.h"
#include "read_file.h"
#include "sorter.h"

#define MAX_INTEGER 2147483647
#define MIN_INTEGER -2147483647

/* put the two large data array on the heap memory */
double *number_data;
int *merged_index;

/* given the No. of sorter, the total No. of data lines, and whether to use random range, 
this function will generate sorting ranges for each sorter, 
and store each sorter's sort starting index and No. of data lines to sort separately in the corresponding position of two arrays */
void get_sort_range(int* start_from, int* sort_ranges, int no_of_worker, int random_range, int no_of_lines){
    int start = 0;

    /* if want to use random range */
    if (random_range){
        int current = 0;
        /* make it random */
        srand(time(NULL));
        /* initialize the worker_sort_ranges and start_from arrays */
        for (int p = 0; p < no_of_worker; p++){
            // we want every worker to have at least 1 data line to sort
            sort_ranges[p] = 1;
            start_from[p] = 0;
        }
        /* then, every time we randomly pick a slot to add one to its sorting range */
        for (int i = 0; i < (no_of_lines - no_of_worker); i++){
            int r = (rand() % (no_of_worker));
            sort_ranges[r] ++;
        }

        /* finally generate the start points based on the obtained sort ranges */
        for (int j = 0; j < no_of_worker; j++){
            start_from[j] = current;
            current += sort_ranges[j];
        }
    }
    /* otherwise, equally distribute work to every worker */
    else{
        int to_sort = no_of_lines / no_of_worker;
        for (int i = 0; i < no_of_worker - 1; i++){
            sort_ranges[i] = to_sort;
            start_from[i] = start;
            start += to_sort;
        }
        /* we have to make sure the last worker's sorting range will fulfill the total data lines' number */
        sort_ranges[no_of_worker - 1] = no_of_lines - (no_of_worker - 1) * to_sort;
        start_from[no_of_worker - 1] = start;
    }
}

/* create the Coordinator node and do Coordinator and Merger's work */
void create_coord(char* file_address, int no_of_worker, int random_range, int attribute_number, int order, char* output_address, int no_of_lines, pid_t root_id){
    
    /* allocate memory for all the data we need */
    /* it stores every line of original data, which can be accessed later by index to write into output file */
    char *all_lines[no_of_lines];
    /* it stores the numeric data parsed from the wanted attribute, making it easier to do sorting */
    number_data = (double*) malloc(no_of_lines * sizeof(double));
    /* it stores the final merged index list done by Merger, can be used to access original data lines and write into output file */
    merged_index = (int*) malloc(no_of_lines * sizeof(int));
    /* all sorter's id array */
    pid_t worker_id[no_of_worker];
    int counter = 0;
    /* array storing all worker's starting point of sort */
    int *worker_starting_point;
    worker_starting_point = (int*) malloc(no_of_worker * sizeof(int));
    /* array storing all worker's sorting ranges */
    int *worker_sort_ranges;
    worker_sort_ranges = (int*) malloc(no_of_worker * sizeof(int));
    /* array storing all fifos' file descriptors, used for polling */
    struct pollfd *pollfds;
    pollfds = (struct pollfd*) malloc(no_of_worker * sizeof(struct pollfd));

    int status;
    pid_t cur_worker;
    int non_exits = no_of_worker;
    /* array of file pointers, so that it can open fifo and read it line by line using fgets() */
    FILE **file_list;
    file_list = (FILE**) malloc(no_of_worker * sizeof(FILE*));
    /* a 2D array, where each entry is a 1D array storing the sorted data's corresponding indices (returned by sorting algorithms), prepared for merge */
    int **index_lists;
    index_lists = (int**) malloc(no_of_worker * sizeof(int*));
    /* a helper array to let the polling part know whether it has read to the end of reading from fifo 
    each of its entry stores an integer representing the counter of how many numeric data it has read from this worker's
    fifo so far */
    int *index_lists_cur;
    index_lists_cur = (int*) malloc(no_of_worker * sizeof(int));
    /* array storing each sorter's reported time to sort */
    double *real_runtime;
    real_runtime = (double*) malloc(no_of_worker * sizeof(double));

    printf("Prepare for data parsing...\n");

    /* get the sorting range for each worker */
    get_sort_range(worker_starting_point, worker_sort_ranges, no_of_worker, random_range, no_of_lines);

    /* separate every line of original data and store them in the array */
    separate_line(file_address, all_lines, no_of_lines);

    /* parse the numeric data wanted to sort and store them into the number data array */
    parse_wanted_num(all_lines, number_data, attribute_number, no_of_lines);

    /* initialize file descriptors for each sorter */
    init_fd_for_worker(pollfds, no_of_worker);

    /* loop to create workers */
    for (counter = 0; counter < no_of_worker; counter++){
        // fork error
        if ((worker_id[counter] = fork()) < 0){
            perror("fork error");
            abort();
        // successfully created a worker
        } else if (worker_id[counter] == 0) {
            int worker_start = worker_starting_point[counter];
            int worker_range = worker_sort_ranges[counter];
            printf("I am sorter #%d, my ID is %d\n", counter, getpid());
            printf("I am going to start from %d, and sort %d items\n", worker_start, worker_range);
            
            /* enter into sorter's working area */
            prepare_and_sort(number_data, counter, worker_start, worker_range, order, root_id);

        }
    }

    /* wait for every worker to exit */
    int wait_counter = 0;
    while (wait_counter < no_of_worker) {
        cur_worker = wait(&status);
        printf("Worker with ID %ld exited with status %x.\n", (long)cur_worker, status>>8);
        wait_counter ++;
    }

    /* the advent of Merger node */
    struct tms merger_start_time_struct, merger_end_time_struct;
    double ticspersec;
    int i, sum = 0;
    double merger_start_time, merger_end_time;
    /* start Merger's working time counting */
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    merger_start_time =  (double) times(&merger_start_time_struct);

    /* initialize some arrays for polling */
    for (int e = 0; e < no_of_worker; e++){
        /* initialize the index lists to store every sorter's working results */
        int *idx_ptr = (int*) malloc(worker_sort_ranges[e] * sizeof(int));
        index_lists[e] = idx_ptr;
        /* initialize the array storing current no. of data pulled by poll */
        index_lists_cur[e] = 0;

        /* initialize the file pointer array for fgets() in polling */
        FILE *file_ptr = (FILE*) malloc(sizeof(FILE));
        file_ptr = fdopen(pollfds[e].fd, "r");
        file_list[e] = file_ptr;
    }

    /* polling function */
    while (1) {
        /* poll the poll file descriptors and pull the finished data */
        int res = poll(pollfds, no_of_worker, 5000);

        /* loop to get data from every worker */
        for (int u=0; u<no_of_worker; u++){

            if (res<0)  { 
                // perror("poll");
                if (errno == EINTR) continue;
            }
            /* if nothing to grab, this means the fifo has reached end, close it */
            if (res==0) { 
                printf("Fifo #%d closed\n", u);
                // close(pollfds[u].fd);
                // fclose means to free the FILE pointer, so we don't need to free it anymore
                // fclose(file_list[u]);
                non_exits --;
            }
            fflush(stdout);

            /* otherwise, the polled data is valid, we can start grabbing sorter's output */
            char mystring [100];
            if (file_list[u] == NULL)
            {   perror ("Error opening fifo");
                exit(1);
            }
            while (fgets(mystring, 100, file_list[u]) != NULL){
                // printf("Pipe %d read %d bytes: '%s'\n", u, res, mystring);
                /* if not reached the designated sorting ranges for this sorter, this means this is still the sorted index */
                if (index_lists_cur[u] < worker_sort_ranges[u]){
                    int cur = atoi(mystring);
                    /* add the index by the worker's starting point, so that the index can fit into the whole data's indices */
                    index_lists[u][index_lists_cur[u]] = cur + worker_starting_point[u];
                    index_lists_cur[u]++;
                    printf("Named Pipe #%d reads: %s", u, mystring);
                } else{
                    /* if exceeds the worker's sorting range, this means it is the reported sorting time for this sorter */
                    real_runtime[u] = atof(mystring);
                }
            }
        }
        if (non_exits == 0) break;
    }
    
    free(pollfds);
    free(worker_starting_point);

    /* merger does merging work */
    printf("Merger starts merging work...\n");
    merge_data(merged_index, number_data, index_lists, no_of_worker, no_of_lines, worker_sort_ranges, order);

    /* end merger's work timing */
    for (i = 0; i < 100000000; i++) sum += i;
    merger_end_time = (double) times(&merger_end_time_struct);
    double merger_run_time = (merger_end_time - merger_start_time) / ticspersec;

    /* merger writes into output file */
    printf("Merger starts to write data into output file...\n");
    FILE *fp = fopen(output_address, "wa");
    if (fp == NULL){
        printf("File cannot be opened\n");
        return;
    }
    for (int k = 0; k < no_of_lines; k++){
        /* iterate through every line and access the corresponding original data line by the merged sorted index */
        fprintf(fp, "%s\n", all_lines[merged_index[k]]);
    }
    fclose(fp);
    printf("Merger finishes writing.\n");

    /* send sigusr2 to Root */
    sleep(1);
    kill(root_id, SIGUSR2);
    sleep(2);

    /* report timing stats */
    printf("Timing Statistics:\n");
    sleep(1);

    for (int i = 0; i < no_of_worker; i++){
        printf("Real runtime (sec) for sorter #%d: %f\n", i, real_runtime[i]);
    }
    printf("Real runtime (sec) for merger: %f\n", merger_run_time);

    /* before exiting, release all allocated memory */
    free(worker_sort_ranges);
    for (int i = 0; i < no_of_worker; i++) free(index_lists[i]);
    free(index_lists);
    for (int j = 0; j < no_of_lines; j++) free(all_lines[j]);
    free(number_data);
    free(merged_index);
    free(file_list);
    free(index_lists_cur);
    free(real_runtime);

    exit(0);
}

/* responsible for merging a multiple of sorted index arrays into one large merged index array, 
where each index indicates the corresponding data line's position in the sorted numeric field */
void merge_data(int *merged_index, double *number_data, int **index_lists, int no_of_worker, int no_of_lines, int *sort_ranges, int order){
    
    /* this pointer array will point to the current "head" of every sorted index arrays, so that
    we can compare each "head" and pick one at a time to do the merge */
    int *pointer_arr;
    pointer_arr = (int*) malloc(no_of_worker * sizeof(int));
    /* initialize the "pointers" */
    for (int i = 0; i < no_of_worker; i++){
        pointer_arr[i] = 0;
    }

    if (order){
        /* descending */
        for (int j = 0; j < no_of_lines; j++){
            /* in every iteration, we will find out a "largest value" */
            double largest_val = MIN_INTEGER;
            int u;
            int large_idx;
            int largest_val_idx;
            /* compare every sorted index array's current index's corresponding data value pointed by "pointer" */
            for (u = 0; u < no_of_worker; u++){
                int current_pointer = pointer_arr[u];
                /* first, make sure that the "pointer" points to a value within the sorter's sorting range;
                otherwise, this exceeds the sorting range and is not a valid number, meaning that this sored index
                list has already being emptied */
                if (pointer_arr[u] < sort_ranges[u]){
                    /* access the corresponding index from the sorter's sorted index list */
                    int current_idx = index_lists[u][current_pointer];
                    /* and access the associated numeric data value */
                    double current_number = number_data[current_idx];
                    if (current_number > largest_val){
                        /* if the numeric value is currently largest, update the largest value for this round,
                        and note down the index, which we will be writing into merged_index */
                        largest_val = current_number;
                        large_idx = u;
                        largest_val_idx = current_idx;
                    }
                }
            }
            /* write the largest index for this round into merged_index, and pass the pointer to the next index in this sorted index array */
            merged_index[j] = largest_val_idx;
            pointer_arr[large_idx] ++;
        }
    } else {
        /* ascending, similar to descending but we are finding the smallest value in each iteration */
        for (int j = 0; j < no_of_lines; j++){
            double smallest_val = MAX_INTEGER;
            int u;
            int small_idx;
            int smallest_val_idx;
            for (u = 0; u < no_of_worker; u++){
                int current_pointer = pointer_arr[u];
                if (pointer_arr[u] < sort_ranges[u]){
                    int current_idx = index_lists[u][current_pointer];
                    double current_number = number_data[current_idx];
                    if (current_number < smallest_val){
                        smallest_val = current_number;
                        small_idx = u;
                        smallest_val_idx = current_idx;
                    }
                }
            }
            merged_index[j] = smallest_val_idx;
            pointer_arr[small_idx] ++;
        }
    }
    /* free the pointer array */
    free(pointer_arr);
}

/* handler function for sigusr2 */
void sig2_handler(int signum){
    /* re-claiming the handler for future use */
    signal(SIGUSR2, sig2_handler);
    if (signum == SIGUSR2){
        /* if the signal is sigusr2, increment the counter */
        no_of_sig2 ++;
    }
}

