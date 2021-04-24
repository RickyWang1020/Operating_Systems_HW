#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include "chef_help.h"

int main(int argc, char *argv[]){
    /* inline parameter variables */
    int no_of_salad;
    int chef_time;
     /* an array containing 3 salad makers' id */
    pid_t maker_id[3];
    /* the two file names of logs that record the parallel working time log and the saladmakers' temporal log */
    const char *parallel_file = "./parallel_time.txt";
    const char *temporal_log = "./temporal_log.txt";

    /* initialize the timestamp of the starting time of chef process */
    double chef_start_time;
    struct tms chef_start_time_b;
    chef_start_time = (double) times(&chef_start_time_b);
    char chef_start_time_str[100];
    snprintf(chef_start_time_str, 100, "%f", chef_start_time);

    /* these const char strings are the names of semaphores */
    const char *chef_ready_str[3] = {"/chef_ready_0", "/chef_ready_1", "/chef_ready_2"};
    const char *maker_ready_str[3] = {"/maker0_ready", "/maker1_ready", "/maker2_ready"};
    const char *put_onto_workbench = "/put_onto_workbench";
    const char *get_from_workbench = "/get_from_workbench";
    const char *write_into_report = "/write_into_report";
    const char *check_parallel_status = "/check_parallel_status";
    const char *write_maker_log = "/write_maker_log";

    /* because I am running on Mac, it is better to unlink the names of semaphores first, because Mac does not automatically purge semaphores */
    for (int q = 0; q < 3; q++){
        sem_unlink(chef_ready_str[q]);
        sem_unlink(maker_ready_str[q]);
    }
    sem_unlink(put_onto_workbench);
    sem_unlink(get_from_workbench);
    sem_unlink(write_into_report);
    sem_unlink(check_parallel_status);
    sem_unlink(write_maker_log);

    /* create semaphores */
    /* this semaphore array stores the semaphores indicating whether the chef is ready for giving vegetables to the i-th saladmaker */
    sem_t *chef_ready_sem[3];
    /* this semaphore array stores the semaphores indicating whether the i-th saladmaker is ready to receive vegetables from chef */
    sem_t *maker_ready_sem[3];
    /* this semaphore indicates whether the chef has put vegetables onto the workbench for specific saladmaker to receive */
    sem_t *put_onto_workbench_sem;
    if ( (put_onto_workbench_sem = sem_open(put_onto_workbench, O_CREAT | O_EXCL, 0777, 0)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    /* this semaphore indicates whether the saladmaker has finished receiving the two vegetables from chef */
    sem_t *get_from_workbench_sem;
    if ( (get_from_workbench_sem = sem_open(get_from_workbench, O_CREAT | O_EXCL, 0777, 0)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    /* this semaphore makes sure that only one saladmaker at a time can access/ write into the result statistics report shared memory */
    sem_t *write_into_report_sem;
    if ( (write_into_report_sem = sem_open(write_into_report, O_CREAT | O_EXCL, 0777, 1)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    /* this semaphore lets the saladmaker check whether they are in a parallel working status, and potentially write into the parallel working log */
    sem_t *check_parallel_status_sem;
    if ( (check_parallel_status_sem = sem_open(check_parallel_status, O_CREAT | O_EXCL, 0777, 1)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    /* this semaphore makes sure that only one saladmaker at a time can access/ write into the temporal log of saladmakers' work */
    sem_t *write_maker_log_sem;
    if ( (write_maker_log_sem = sem_open(write_maker_log, O_CREAT | O_EXCL, 0777, 1)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }

    /* initialize the two semaphore arrays */
    /* since the chef needs to wait for the saladmaker to get ready, and the saladmaker has to wait for the chef to get ready, the semaphore needs to have initial value 0 */
    for (int n = 0; n < 3; n++){
        // chef_ready_sem[n] = sem_open(chef_ready_str[n], O_CREAT | O_EXCL, 0777, 0);
        if ( (chef_ready_sem[n] = sem_open(chef_ready_str[n], O_CREAT | O_EXCL, 0777, 0)) == SEM_FAILED) {
            perror("sem_open:");
            exit(-1);
        }
    }
    for (int m = 0; m < 3; m++){
        // maker_ready_sem[m] = sem_open(maker_ready_str[m], O_CREAT | O_EXCL, 0777, 0);
        if ( (maker_ready_sem[m] = sem_open(maker_ready_str[m], O_CREAT | O_EXCL, 0777, 0)) == SEM_FAILED) {
            perror("sem_open:");
            exit(-1);
        }
    }

    /* create shared memory blocks */
    /* create a piece of shared memory to work as the workbench, where the chef places her picked vegetables and waits for saladmaker to pick */
    int workbench_shm_id = shmget(IPC_PRIVATE, 6 * sizeof(int), IPC_CREAT|0777);
    if (workbench_shm_id == -1) {
        perror("shmget workbench_shm_id: ");
        exit(1);
    }
    /* attach the shared memory to the "workbench" array */
    /* the array has six integer slots, the first two slots will put the vegetables that maker0 will receive, the next two slots will put maker1's, and the final two slots will put maker2's */
    /* I set maker0 has only onions, therefore his two slots will put tomato and pepper */
    /* I set maker1 has only peppers, therefore his two slots will put onion and tomato */
    /* I set maker2 has only tomatoes, therefore his two slots will put onion and pepper */
    int *workbench_shm_arr = (int *)shmat(workbench_shm_id, NULL, 0);
    if (!workbench_shm_arr) { /* operation failed. */
        perror("shmat workbench_shm_arr: ");
        exit(1);
    }
    /* initialize the workbench array as all zeros, meaning that no vegetables are placed */
    for(int i = 0; i < 6; i++){
        workbench_shm_arr[i] = 0;
    }

    /* create another piece of shared memory to store the reported results of all saladmakers */
    /* the results' structure is defined in the struct report_result in chef_help.h */
    int results_shm_id = shmget(IPC_PRIVATE, sizeof(result_report), IPC_CREAT|0777);
    if (results_shm_id == -1) {
        perror("shmget results_shm_id: ");
        exit(1);
    }
    /* attach the shared memory to the result report struct */
    result_report *results_shm_struct = (result_report *)shmat(results_shm_id, NULL, 0);
    if (!results_shm_struct) { /* operation failed. */
        perror("shmat results_shm_struct: ");
        exit(1);
    }

    /* create a shared memory to keep track of the parallel working time */
    /* the parallel working tracker's structure is defined in the struct parallel_working in chef_help.h */
    int parallel_shm_id = shmget(IPC_PRIVATE, sizeof(parallel_working), IPC_CREAT|0777);
    if (parallel_shm_id == -1) {
        perror("shmget parallel_shm_id: ");
        exit(1);
    }
    /* attach the shared memory to the parallel working struct */
    parallel_working *parallel_shm_struct = (parallel_working *)shmat(parallel_shm_id, NULL, 0);
    if (!parallel_shm_struct) { /* operation failed. */
        perror("shmat parallel_shm_struct: ");
        exit(1);
    }

    /* initialize the two log files */
    FILE *fp;
    fp = fopen(parallel_file, "wa");
    if (fp == NULL){
        printf("File cannot be opened\n");
        return -1;
    }
    fprintf(fp, "The list of time periods that 2 or more saladmakers were busy at the same time (working in parallel)\n");
    fprintf(fp, "Taking the Timestamp that Chef process starts as the timing zero point, time unit is second\n");
    fclose(fp);

    FILE *fp_log = fopen(temporal_log, "wa");
    if (fp_log == NULL){
        printf("File cannot be opened\n");
        return -1;
    }
    fprintf(fp_log, "The temporal log that reveals the timeline operation of each saladmaker\n");
    fprintf(fp_log, "Taking the Timestamp that Chef process starts as the timing zero point, time unit is second\n");
    fclose(fp_log);

    /* the chef starts working */
    printf("Hello I am the chef\n");
    /* take the inline parameters */
    for (int c = 1; c < argc; c++){
        /* no_of_salad is an integer indicated by -n flag, representing the no. of salad we want */
        if (strcmp(argv[c], "-n") == 0){
            no_of_salad = atoi(argv[c+1]);
            if (no_of_salad <= 0){
                printf("Invalid # of salads!\n");
                return 1;
            }
        }
        /* chef_time is an integer indicated by -m flag, indicating the max resting time for chef in between preparing each salad */
        if (strcmp(argv[c], "-m") == 0){
            chef_time = atoi(argv[c+1]);
            if (chef_time <= 0){
                printf("Invalid chef break time!\n");
                return 1;
            }
        }
    }

    /* generate the actual resting time of chef after delivering vegetables each time: [0.5 * cheftime, cheftime] */
    int wait_time = generate_chef_break_time(chef_time);

    /* receive the user input of max salad-making time for each saladmaker */
    int maker_time[3];
    for (int n = 0; n < 3; n++){
        printf("Enter the maximum salad-making time for maker #%d:\n", n+1);
        scanf("%d", &maker_time[n]);
    }

    /* create the three salad makers as the chef process's children */
    for (int counter = 0; counter < 3; counter ++){
        // fork error
        if ((maker_id[counter] = fork()) < 0){
            perror("fork error");
            abort();
        }
        // successfully created a salad maker
        else if (maker_id[counter] == 0){
            printf("I am salad maker #%d, my ID is %d\n", counter, getpid());
            /* convert all the numerical values into strings so as to send into exec */
            char maker_time_str[5];
            snprintf(maker_time_str, 5, "%d", maker_time[counter]);
            char counter_str[5];
            snprintf(counter_str, 5, "%d", counter);
            char workbench_shm_str[10];
            snprintf(workbench_shm_str, 10, "%d", workbench_shm_id);
            char results_shm_str[10];
            snprintf(results_shm_str, 10, "%d", results_shm_id);
            char parallel_shm_str[10];
            snprintf(parallel_shm_str, 10, "%d", parallel_shm_id);
            /* index for argv[]: 2: max saladmaking time, 4: chef starting time timestamp, 6: the index (0,1,2) of this saladmaker, 8: the shmid of workbench, 10: the shmid of result report, 12: the shmid of parallel working */
            char *maker_arg[] = {"./saladmaker", "-m", maker_time_str, "-t", chef_start_time_str, "-i", counter_str, "-w", workbench_shm_str, "-r", results_shm_str, "-p", parallel_shm_str, NULL};
            execvp("./saladmaker", maker_arg);
            exit(0);
        }
    }

    for (int salad = 1; salad <= no_of_salad; salad ++){
        /* the chef randomly grabs 2 kinds of vegetables */
        /* first, decide which 2 kinds of vegetable to grab */
        /* generate a random number from 0 to 2, 0 means do not grab onion (and send vegetables to saladmaker 0), 1 means do not grab pepper (and send vegetables to saladmaker 1), 2 means do not grab tomato (and send vegetables to saladmaker 1) */
        srand(time(NULL));
        int veg_idx = (rand() % 3);
        printf("chef: I will send vegetables to maker%d\n", veg_idx);

        /* the chef starts her working */
        printf("chef: I, chef, am ready\n");
        fflush(stdout);
        /* the chef indicates that she is ready */
        sem_post(chef_ready_sem[veg_idx]);
        printf("chef: maker%d, are you ready\n", veg_idx);
        fflush(stdout);
        /* the chef waits for the corresponding saladmaker to get ready */
        if (sem_wait(maker_ready_sem[veg_idx]) < 0) {
            perror("maker_ready_sem wait failed");
            break;
        }
        printf("chef: time to grab vegetables and put onto workbench\n");
        fflush(stdout);
        /* grab the 2 kinds of vegetables */
        if (veg_idx == 0){
            /* saladmaker 0 only has onion, give him tomato and pepper */
            int tomato_weight = grab_tomato();
            int pepper_weight = grab_pepper();
            /* put the vegetable into shared memory */
            workbench_shm_arr[0] = tomato_weight;
            workbench_shm_arr[1] = pepper_weight;
            printf("chef: tomato: %d, pepper: %d\n", tomato_weight, pepper_weight);
        }
        else if (veg_idx == 1){
            /* saladmaker 1 only has pepper, give him tomato and onion */
            int tomato_weight = grab_tomato();
            int onion_weight = grab_onion();
            /* put the vegetable into shared memory */
            workbench_shm_arr[2] = onion_weight;
            workbench_shm_arr[3] = tomato_weight;
            printf("chef: onion: %d, tomato: %d\n", onion_weight, tomato_weight);
        }
        else {
            /* saladmaker 1 only has tomato, give him onion and pepper */
            int onion_weight = grab_onion();
            int pepper_weight = grab_pepper();
            /* put the vegetable into shared memory */
            workbench_shm_arr[4] = onion_weight;
            workbench_shm_arr[5] = pepper_weight;
            printf("chef: onion: %d, pepper: %d\n", onion_weight, pepper_weight);
        }
        /* the chef notifies the saladmaker that she finishes sending vegetables */
        sem_post(put_onto_workbench_sem);
        printf("chef: wait for you to get from workbench\n");
        fflush(stdout);
        /* the chef waits this saladmaker to pick up vegetables from workbench */
        if (sem_wait(get_from_workbench_sem) < 0) {
            perror("get_from_workbench_sem wait failed");
            break;
        }
        /* after the saladmaker picks up the vegetables, the chef will rest a bit and go back to work loop again */
        printf("chef: I will rest a bit, just %d seconds\n", wait_time);
        fflush(stdout);
        sleep(wait_time);
    }

    /* after the number of salads is enough, the chef sets the shm to all -1, so as to notify all salad makers that the work is finished */
    /* first, wait until there are no processes waiting for picking vegetables */
    int sem_val;
    while (sem_getvalue(put_onto_workbench_sem, &sem_val) > 0){
        if (sem_val == 0) break;
    }
    printf("Good! No process waiting for the workbench\n");
    /* then, change all vegetables to -1 */
    for(int i=0; i<6; i++){
        workbench_shm_arr[i] = -1;
    }
    /* notify all saladmakers to quit */
    for (int c = 0; c < 3; c++){
        printf("chef: ready to let saladmaker #%d quit\n", c);
        fflush(stdout);
        sem_post(chef_ready_sem[c]);
        printf("chef: maker%d, are you ready\n", c);
        fflush(stdout);
        if (sem_wait(maker_ready_sem[c]) < 0) {
            perror("maker_ready_sem wait failed");
            break;
        }
        sem_post(put_onto_workbench_sem);
        printf("chef: see the 'vegetables', is that unusual?\n");
        fflush(stdout);
        if (sem_wait(get_from_workbench_sem) < 0) {
            perror("get_from_workbench_sem wait failed");
            break;
        }
    }

    /* wait for every saladmaker processes to exit from exec */
    int wait_counter = 0;
    int status;
    pid_t now_maker;
    while (wait_counter < 3) {
        now_maker = wait(&status);
        printf("Saladmaker #%d with ID %ld exited with status %x.\n", wait_counter, (long)now_maker, status>>8);
        wait_counter ++;
    }

    /* access the parallel working time log and add the final accumulated statistics of total parallel working time */
    /* first, wait until no saladmakers are waiting for accessing the parallel working shared memory and the log file (just for debug purpose) */
    int sem_parallel_val;
    while (sem_getvalue(check_parallel_status_sem, &sem_parallel_val) > 0){
        if (sem_parallel_val == 0) break;
    }
    printf("Good! No process waiting for writing into parallel recording struct\n");
    /* next, open the log file and write the total parallel working time at the last line of file */
    fp = fopen(parallel_file, "a");
    if (fp == NULL){
        printf("File cannot be opened\n");
        return -1;
    }
    fprintf(fp, "Total amount of parallel working time: %f (seconds)\n", parallel_shm_struct->parallel_time_accumulator);
    fclose(fp);

    /* print out the result report of statistics by calling the print_results_shm function */
    /* first, wait until no saladmakers are waiting for accessing the result report shared memory and the log file (just for debug purpose) */
    int sem_writer_val;
    while (sem_getvalue(write_into_report_sem, &sem_writer_val) > 0){
        if (sem_writer_val == 0) break;
    }
    printf("Good! No process waiting for writing into working results\n");
    print_results_shm(results_shm_struct);

    /* detach the shared memory segment from our process's address space */
    if (shmdt(workbench_shm_arr) == -1) {
        perror("shmdt workbench_shm_arr: ");
    }
    if (shmdt(results_shm_struct) == -1) {
        perror("shmdt results_shm_struct: ");
    }
    if (shmdt(parallel_shm_struct) == -1) {
        perror("shmdt parallel_shm_struct: ");
    }

    /* de-allocate the shared memory segment */
    if (shmctl(workbench_shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl workbench_shm_id: ");
    }
    if (shmctl(results_shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl results_shm_id: ");
    }
    if (shmctl(parallel_shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl parallel_shm_id: ");
    }

    /* before leaving, the chef needs to first clost all the semaphores */
    for (int ctr = 0; ctr < 3; ctr++){
        sem_close(chef_ready_sem[ctr]);
        sem_close(maker_ready_sem[ctr]);
    }
    sem_close(put_onto_workbench_sem);
    sem_close(get_from_workbench_sem);
    sem_close(write_into_report_sem);
    sem_close(check_parallel_status_sem);
    sem_close(write_maker_log_sem);

    /* next, the chef needs to unlink the named semaphores */
    for (int ctr2 = 0; ctr2 < 3; ctr2 ++){
        sem_unlink(chef_ready_str[ctr2]);
        sem_unlink(maker_ready_str[ctr2]);
    }
    sem_unlink(put_onto_workbench);
    sem_unlink(get_from_workbench);
    sem_unlink(write_into_report);
    sem_unlink(check_parallel_status);
    sem_unlink(write_maker_log);

    return 0;
}