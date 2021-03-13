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

/* A small function to help getting the min between two integers */
int min(int x, int y) {
    return (x < y) ? x : y;
}

/* the function to merge two sorted arrays */
void merge(double *val_arr, int *idx_arr, int left_start, int mid, int right_end, int order){ 
    int left_counter, right_counter; 
    int left_len = mid - left_start + 1;
    int right_len =  right_end - mid;
  
    /* create some temperal arrays to store the current state of original arrays
    because we will change on the original data file in the following executions */
    double *left_val, *right_val;
    int *left_idx, *right_idx;

    /* val array stores the value, idx array stores the corresponding index of this value in the original array
    we use idx array to help track the index, so that we can pass the sorted data's index out to Merger, and we can
    just use indexing to access lines in the original data file */
    left_val = (double*) malloc(left_len * sizeof(double));
    right_val = (double*) malloc(right_len * sizeof(double));
    left_idx = (int*) malloc(left_len * sizeof(int));
    right_idx = (int*) malloc(right_len * sizeof(int));
    
    /* Copy parts of current data array to the temperal arrays */
    for (left_counter = 0; left_counter < left_len; left_counter++) {
        left_val[left_counter] = val_arr[left_start + left_counter];
        left_idx[left_counter] = idx_arr[left_start + left_counter];
    }
    for (right_counter = 0; right_counter < right_len; right_counter++) {
        right_val[right_counter] = val_arr[mid + 1 + right_counter];
        right_idx[right_counter] = idx_arr[mid + 1 + right_counter];
    }
  
    left_counter = 0; // Position pointer of left subarray 
    right_counter = 0; // Position pointer of right subarray 
    int arr_counter = left_start; // Initial index of merged subarray, so that we will know where to "overwrite" our sorted result on the original arrays

    /* merge the data in descending order */
    if (order){
        while ((left_counter < left_len) && (right_counter < right_len)){ 
            /* if the current number in left array is larger, overwrite this number and its index separately to position in val and idx array */
            if (left_val[left_counter] >= right_val[right_counter]){ 
                val_arr[arr_counter] = left_val[left_counter]; 
                idx_arr[arr_counter] = left_idx[left_counter]; 
                left_counter++; 
            } else{ 
                /* otherwise, overwrite right array's value and index into arrays */
                val_arr[arr_counter] = right_val[right_counter]; 
                idx_arr[arr_counter] = right_idx[right_counter]; 
                right_counter++; 
            } 
            arr_counter++; 
        } 
    
        /* Copy the remaining elements of left array, if there are any */
        while (left_counter < left_len) { 
            val_arr[arr_counter] = left_val[left_counter]; 
            idx_arr[arr_counter] = left_idx[left_counter]; 
            left_counter++; 
            arr_counter++; 
        } 
    
        /* Copy the remaining elements of right array, if there are any */
        while (right_counter < right_len) { 
            val_arr[arr_counter] = right_val[right_counter]; 
            idx_arr[arr_counter] = right_idx[right_counter]; 
            right_counter++; 
            arr_counter++; 
        } 
    } else{
        /* merge the data in ascending order, it is similar to descending, but just change the comparison direction */
        while ((left_counter < left_len) && (right_counter < right_len)){ 
            if (left_val[left_counter] <= right_val[right_counter]){ 
                val_arr[arr_counter] = left_val[left_counter]; 
                idx_arr[arr_counter] = left_idx[left_counter]; 
                left_counter++; 
            } else{ 
                val_arr[arr_counter] = right_val[right_counter]; 
                idx_arr[arr_counter] = right_idx[right_counter]; 
                right_counter++; 
            } 
            arr_counter++; 
        } 
    
        /* Copy the remaining elements of left array, if there are any */
        while (left_counter < left_len) { 
            val_arr[arr_counter] = left_val[left_counter]; 
            idx_arr[arr_counter] = left_idx[left_counter]; 
            left_counter++; 
            arr_counter++; 
        } 
    
        /* Copy the remaining elements of right array, if there are any */
        while (right_counter < right_len) { 
            val_arr[arr_counter] = right_val[right_counter]; 
            idx_arr[arr_counter] = right_idx[right_counter]; 
            right_counter++; 
            arr_counter++; 
        } 
    }
    
} 
  
/* the mergesort function to iteratively perform merge to get a sorted array
and its corresponding index as another index array */
void merge_sort(double *value_arr, int* index_arr, int arr_len, int order){
    int curr_size;  // the current size of subarrays to do merge, curr_size varies from 1 to n/2
    int left_start; // the starting index of left array
  
    /* merge subarrays of increasing sizes. First merge subarrays of 
    size 1 to create sorted subarrays of size 2, then merge subarrays
    of size 2 to create sorted subarrays of size 4, and so on. */
    for (curr_size = 1; curr_size <= arr_len - 1; curr_size = 2 * curr_size){
        /* partition the origial array into several left + right subarrays, each left + right subarray have size of 2*cur_size,
        so that left and right subarray will have size of cur_size ideally, but it can be shorter because the length of array
        may not be the power of 2 */
        for (left_start = 0; left_start < arr_len - 1; left_start += 2 * curr_size){

            /* mid is the end of left array, mid+1 is the start of right array */
            int mid = min(left_start + curr_size - 1, arr_len - 1);
            /* right array and left array have the same size - cur_size,
            and we also need to check if it exceeds the length of array */
            int right_end = min(left_start + 2 * curr_size - 1, arr_len - 1);

            /* perform merge on left array (left_start ~ mid) and right array (mid+1 ~ right_end) */
            merge(value_arr, index_arr, left_start, mid, right_end, order);
       }
   }
}

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
    
    /* merge sort part */
    merge_sort(value_list, index_list, list_length, order);

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
