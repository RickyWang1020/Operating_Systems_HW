#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/times.h>

int main(int argc, char** argv){
    /* from the passed arguments, grab the corresponding variables */
    char* worker_idx = malloc(10);
    strcpy(worker_idx, argv[1]);

    int list_length = atoi(argv[2]);
    int order = atoi(argv[3]);
    int root_id = atoi(argv[4]);

    /* starts to count the working time of sorter */
    double worker_start_time, worker_end_time;
    struct tms worker_start_time_struct, worker_end_time_struct;
    double ticspersec;
    int i, sum = 0;

    ticspersec = (double) sysconf(_SC_CLK_TCK);
    worker_start_time = (double) times(&worker_start_time_struct);

    /* initialize the value list to store the numeric values for sorting,
    and index list to store the indices of these values, we will change the
    order of these values as well as the order their corresponding indices
    in the sorting process */
    double *value_list;
    int *index_list;

    value_list = (double*) malloc(list_length * sizeof(double));
    index_list = (int*) malloc(list_length * sizeof(int));

    for (int i = 0; i < list_length; i++){
        index_list[i] = i;
        value_list[i] = atof(argv[5+i]);
    }

    /* Bubble sort part */
    if (order){
        /* in descending order */
        for (int c = 0 ; c < list_length - 1; c++){
            for (int d = 0 ; d < list_length - c - 1; d++){
                /* we compare every pair of data, and exchange their order
                if the left one is smaller than the right one */
                if (value_list[d] < value_list[d+1]){
                    int val_temp = value_list[d];
                    value_list[d]   = value_list[d+1];
                    value_list[d+1] = val_temp;
                    /* also exchange their order in index list */
                    int index_temp = index_list[d];
                    index_list[d]   = index_list[d+1];
                    index_list[d+1] = index_temp;
                }
            }
        }
    }
    else {
        /* in ascending order */
        for (int c = 0 ; c < list_length - 1; c++){
            for (int d = 0 ; d < list_length - c - 1; d++){
                /* we compare every pair of data, and exchange their order
                if the left one is larger than the right one */
                if (value_list[d] > value_list[d+1]){
                    int val_temp = value_list[d];
                    value_list[d]   = value_list[d+1];
                    value_list[d+1] = val_temp;
                    int index_temp = index_list[d];
                    index_list[d]   = index_list[d+1];
                    index_list[d+1] = index_temp;
                }
            }
        }
    }

    /* generate the fifo name to open the fifo */
    char myfifo[100] = "worker";
    strcat(myfifo, worker_idx); 
 
    int msg_sent = 0;
    char msg[100];
    int fd;
    /* save the stdout fd so that we can later change back to it */
    int saved_stdout = dup(1);
    
    printf("Worker #%s starts to write...\n", worker_idx);
    fflush(stdout);

    if ((fd = open(myfifo, O_RDWR|O_NONBLOCK))<0){
        perror("fifo open error");
        exit(1);
    }
    else{
        /* redirect the stdout fd to our file descriptor, so that whatever we print will be printed into the fifo */
        dup2(fd, 1);
        while (msg_sent < list_length){
            
            printf("%d\n", index_list[msg_sent]);
            fflush(stdout);
            
            msg_sent ++;
        }

    /* end sorter's sorting time */
    for (i = 0; i < 10000000; i++) sum += i;
    worker_end_time = (double) times(&worker_end_time_struct);
    double worker_run_time = (worker_end_time - worker_start_time) / ticspersec;

    printf("%f\n", worker_run_time);

    /* redirect the stdout file descriptor back */
    dup2(saved_stdout, 1);
    close(fd);
    fflush(stdout);

    /* send sigusr1 to root */
    kill(root_id, SIGUSR1);
    sleep(1);
    
    }
    /* release allocated memory */
    free(value_list);
    free(index_list);
    free(worker_idx);
    exit(0);
    return 0;
}
