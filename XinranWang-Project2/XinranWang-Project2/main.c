#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/times.h>
#include "read_file.h"
#include "node.h"
#include "sorter.h"

/* global variable indicating no. of sigusr1/ sigusr2 seen by the Root */
int no_of_sig1 = 0;
int no_of_sig2 = 0;

/* The main function of this program, doing the work of Root node */
int main(int argc, char** argv){

    /* set up the timing variables and starts to track the turnaround time */
    double start_time, end_time;
    struct tms start_time_struct, end_time_struct;
    double ticspersec;
    int i, sum = 0;
    
    ticspersec = (double) sysconf(_SC_CLK_TCK);
    start_time =  (double) times(&start_time_struct);

    char* file_address;
    int no_of_worker;
    int random_range = 0;
    int attribute_number;
    int order;
    char* output_address;
    int no_of_lines;

    /* parse some important variables from flags */
    for (int c = 1; c < argc; c ++){

        /* get the original data file's address */
        if (strcmp(argv[c], "-i") == 0){
            file_address = argv[c+1];
        }
        /* get the no. of sorter */
        if (strcmp(argv[c], "-k") == 0){
            no_of_worker = atoi(argv[c+1]);
        }
        /* check whether the user wants random sorting ranges */
        if (strcmp(argv[c], "-r") == 0){
            random_range = 1;
        }
        /* get the attribute domain for sorting */
        if (strcmp(argv[c], "-a") == 0){
            attribute_number = atoi(argv[c+1]);
            /* if the attribute domain is invalid, abort */
            if ((attribute_number != 0) && (attribute_number != 3) && (attribute_number != 4) && (attribute_number != 5)){
                printf("Invalid attribute number!\n");
                return -1;
            }
        }
        /* get the sorting order wanted by the user */
        if (strcmp(argv[c], "-o") == 0){
            /* ascending: 0; descending: 1 */
            if (strcmp(argv[c+1], "d") == 0) order = 1;
            else if (strcmp(argv[c+1], "a") == 0) order = 0;
            else {
                printf("Invalid sorting order!\n");
                return -1;
            }
        }
        /* get the output file's address */
        if (strcmp(argv[c], "-s") == 0){
            output_address = argv[c+1];
        }
	}

    /* get the no. of lines in this data file */
    no_of_lines = get_no_of_lines(file_address);

    printf("Data Address: %s\n", file_address);
    printf("# of worker: %d\n", no_of_worker);
    printf("Random or not: %d\n", random_range);
    printf("Attribute domain: %d\n", attribute_number);
    printf("Sorting order: %d\n", order);
    printf("Output Address: %s\n", output_address);
    printf("No of lines: %d\n", no_of_lines);

    /* Error check: if the worker number is not valid: <= 0 or > no. of lines */
    if ((no_of_worker <= 0) || (no_of_worker > no_of_lines)){
        printf("Invalid No. of workers!\n");
        return -1;
    }

    /* Error check: if each worker has on average too many lines to sort, abort */
    if ((no_of_lines / no_of_worker > 1500)){
        printf("Too few workers! They will be too tired to sort!\n");
        return -1;
    /* Error check: if there are too many sorters, fifo open error will happen, therefore abort */
    } else if (no_of_worker > 2200){
        printf("No need to dispatch this many workers! Try reducing worker's number!\n");
        return -1;
    }

    pid_t coord_id;
    int status;

    /* the root starts to wait for incoming sigusr1 and sigusr2 */
    signal(SIGUSR1, sig1_handler);
    signal(SIGUSR2, sig2_handler);

    if ((coord_id = fork()) < 0) {
        perror("fork error");
        exit(1);
    }
    /* the Root creates Coordinator node */
    if (coord_id == 0){
        printf("Hey I am the coordinator with id %d\n", getpid());
        create_coord(file_address, no_of_worker, random_range, attribute_number, order, output_address, no_of_lines, getppid());
    }
    /* the Root continues its work */
    else{
        printf("Hey I am the root with id %d\n", getpid());
        /* Root waits for Coordinator to return */
        if (wait(&status)!= coord_id) {
            perror("wait");
            exit(1);
        }
        printf("Coordinator %d terminated with exit code %d\n", coord_id, status>>8);

        /* Root ends time tracking, and reports turnaround time for the program */
        for (i = 0; i < 100000000; i++) sum += i;
        end_time =  (double) times(&end_time_struct);
        double turnaround_time = (end_time - start_time) / ticspersec;
        printf("Turnaround time (sec) for sorting program: %f\n", turnaround_time);

        printf("No. of SIGUSR1 received by Root: %d\n", no_of_sig1);
        printf("No. of SIGUSR2 received by Root: %d\n", no_of_sig2);
    }
    return 0;

}

