#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include "sorter.h"
#include "read_file.h"

/* the working area for a single Sorter node */
void prepare_and_sort(double *all_data_for_read, int sorter_idx, int start, int range, int order, int root_id){

    /* data array used to store some data used for passing into the sorting exec file */
    char **my_data;
    my_data = (char**) malloc((range+5)*sizeof(char*));

    /* create char* holders and convert the numeric variables into char* */
    char idx_char[10], range_char[10], order_char[10], root_id_char[20];
    snprintf(idx_char, 10, "%d", sorter_idx);
    snprintf(range_char, 10, "%d", range);
    snprintf(order_char, 10, "%d", order);
    snprintf(root_id_char, 20, "%d", root_id);
    if (sorter_idx % 2){
        // if the index is odd, use bubble sort
        alloc_mem_for_str(my_data, 0, "bubble_sort", 15);
    } else{
        // if the index is even, use merge sort
        alloc_mem_for_str(my_data, 0, "merge_sort", 15);
    }
    /* then allocate memory for those string and put them in the given slots in the data array */
    alloc_mem_for_str(my_data, 1, idx_char, 10);
    alloc_mem_for_str(my_data, 2, range_char, 10);
    alloc_mem_for_str(my_data, 3, order_char, 10);
    alloc_mem_for_str(my_data, 4, root_id_char, 20);

    /* now put the numeric data for sorting into the data array */
    for (int counter = 0; counter < range; counter++){
        /* create char* holders and convert the numeric data to char* */
        char num_char[50];
        snprintf(num_char, 50, "%f", all_data_for_read[start+counter]);
        /* allocate memory for the string and put it in the given slots in the data array */
        alloc_mem_for_str(my_data, counter+5, num_char, 50);
    }
    /* put null pointer at the end of data array */
    my_data[range+5] = NULL;
    printf("Worker #%d finishes reading data.\n", sorter_idx);

    /* pass the data array to the exec sorting algorithm */
    int ret;
    if (sorter_idx % 2){
        // if the index is odd, use bubble sort
        if ((ret = execvp("./bubble_sort", my_data)) != 0) perror("error in exec");
    } else{
        // if the index is even, use merge sort
        if ((ret = execvp("./merge_sort", my_data)) != 0) perror("error in exec");
    }
    
}

/* initialize all file descriptors that will be used by sorters and read by Merger */
void init_fd_for_worker(struct pollfd *pollfds, int worker){
    for (int i=0; i<worker; i++){
        /* generate the name of each fifo: "worker" + sorter's index */
        char myfifo[20] = "worker";
        char index[10];
        snprintf(index, 10, "%d", i);
        strcat(myfifo, index); 

        /* make fifo for this worker */
        if ((mkfifo(myfifo, S_IWUSR | S_IRUSR | S_IRGRP)) == -1) {
            if (errno != EEXIST){
                perror("Error: couldn't create myfifo pipe");
                exit(0);
            }
        }

        /* open this fifo and assign its attributes to the corresponding position in the pollfd array
        so that further poll function can read from it */
        int fd = open(myfifo, O_RDWR | O_NONBLOCK);
        pollfds[i].fd = fd;
        pollfds[i].events = POLLIN;
        pollfds[i].revents = 0;
    
    }
}

/* handler function for sigusr1 */
void sig1_handler(int signum){
    /* re-claiming the handler for future use */
    void sig1_handler(int signum);
    if (signum == SIGUSR1){
        /* if the signal is sigusr1, increment the counter */
        no_of_sig1 ++;
    }
}
