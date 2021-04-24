#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <semaphore.h>
#include <fcntl.h>
#include "saladmaker_help.h"
#include "chef_help.h"

int main(int argc, char *argv[]){
    /* inline parameter variables */
    int maker_idx;
    int mkrtime;
    double chef_start_time;
    int workbench_shmid, results_shmid, parallel_shmid;
    
    /* some accumulators for counting the number of salads made and number of vegetables received and used */
    int salad_made = 0;
    int tomato_weight, pepper_weight, onion_weight;
    int tomato_weight_counter = 0, pepper_weight_counter = 0, onion_weight_counter = 0;

    /* the two file names of logs that record the parallel working time log and the saladmakers' temporal log */
    const char *parallel_file = "./parallel_time.txt";
    const char *temporal_log = "./temporal_log.txt";

    /* time-related variables */
    double tickspersec; tickspersec = (double) sysconf(_SC_CLK_TCK);
    /* used to track the saladmaker's waiting time for chef's vegetables in one iteration of making salad */
    double maker_wait_time_accumulator = 0;
    double maker_wait_time_start, maker_wait_time_end;
    struct tms wait_startb, wait_endb;
    /* used to track the saladmaker's salad-making time in one iteration */
    double maker_make_salad_time_accumulator = 0;
    double maker_make_salad_time_start, maker_make_salad_time_end;
    struct tms salad_startb, salad_endb;
    /* used to track the current timestamp of saladmaker's operation */
    double timestamp;
    struct tms timestampb;

    /* two file pointers for further opening and writing the log files */
    FILE *fp;
    FILE *fp_log;
    
    /* these const char strings are the names of semaphores */
    const char *chef_ready_str[3] = {"/chef_ready_0", "/chef_ready_1", "/chef_ready_2"};
    const char *maker_ready_str[3] = {"/maker0_ready", "/maker1_ready", "/maker2_ready"};
    const char *put_onto_workbench = "/put_onto_workbench";
    const char *get_from_workbench = "/get_from_workbench";
    const char *write_into_report = "/write_into_report";
    const char *check_parallel_status = "/check_parallel_status";
    const char *write_maker_log = "/write_maker_log";

    /* open semaphores: these semaphores are already created by chef.c */
    sem_t *chef_ready_sem;
    sem_t *maker_ready_sem;
    sem_t *put_onto_workbench_sem;
    if ( (put_onto_workbench_sem = sem_open(put_onto_workbench, O_CREAT, 0777, 0)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    sem_t *get_from_workbench_sem;
    if ( (get_from_workbench_sem = sem_open(get_from_workbench, O_CREAT, 0777, 0)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    sem_t *write_into_report_sem;
    if ( (write_into_report_sem = sem_open(write_into_report, O_CREAT, 0777, 10)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    sem_t *check_parallel_status_sem;
    if ( (check_parallel_status_sem = sem_open(check_parallel_status, O_CREAT, 0777, 1)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    sem_t *write_maker_log_sem;
    if ( (write_maker_log_sem = sem_open(write_maker_log, O_CREAT, 0777, 1)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }

    /* take the inline parameters */
    /* mkrtime indicates the max time for the maker to make a salad */
    mkrtime = atoi(argv[2]);
    if (mkrtime <= 0){
        printf("Invalid salad making time!\n");
        exit(-1);
    }
    /* this variable stores the starting timestamp of chef, can be used to track the timeline of saladmaker */
    chef_start_time = atof(argv[4]);
    /* the index of the saladmaker: 0 means he has only onions; 1 means he has only peppers, 2 means he has only tomatoes */
    maker_idx = atoi(argv[6]);
    /* the shared memory ids */
    workbench_shmid = atoi(argv[8]);
    results_shmid = atoi(argv[10]);
    parallel_shmid = atoi(argv[12]);

    /* attach the shared memory to this process's address space */
    int *workbench_shm_array = shmat(workbench_shmid, 0, SHM_RDONLY);
    result_report *result_struct = shmat(results_shmid, 0, 0);
    parallel_working *parallel_struct = shmat(parallel_shmid, 0, 0);
    
    /* after obtaining the index of the saladmaker, can open the corresponding "ready" semaphore for this saladmaker
    because this saladmaker will only be accessing the chef_ready and maker_ready semaphore corresponding to his index */
    if ( (chef_ready_sem = sem_open(chef_ready_str[maker_idx], O_CREAT, 0777, 0)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }
    if ( (maker_ready_sem = sem_open(maker_ready_str[maker_idx], O_CREAT, 0777, 0)) == SEM_FAILED) {
        perror("sem_open:");
        exit(-1);
    }

    /* record the timestamp that the saladmaker is about to start working */
    /* first take the timestamp, then wait for the semaphore to write into log, so that there will be less time-recording delay */
    int sum0 = 0;
    for (int i = 0; i < 100000000; i++) sum0 += i;
    timestamp = (double) times(&timestampb);
    if (sem_wait(write_maker_log_sem) < 0) {
        perror("write_maker_log_sem wait failed");
        exit(-1);
    }
    fp_log = fopen(temporal_log, "a");
    if (fp_log == NULL){
        printf("File cannot be opened\n");
        exit(-1);
    }
    /* write the information into temporal log */
    fprintf(fp_log, "%f: Saladmaker #%d enters working area\n", (timestamp-chef_start_time)/tickspersec, maker_idx);
    fclose(fp_log);
    sem_post(write_maker_log_sem);

    /* the saladmaker starts working */
    while (1){
        printf("maker%d: chef are you ready?\n", maker_idx);
        fflush(stdout);
        /* the saladmaker starts waiting for the chef to deliver vegetables to him */
        /* start the waiting time tracker */
        int sum = 0;
        for (int i = 0; i < 100000000; i++) sum += i;
        maker_wait_time_start = (double) times(&wait_startb);
        /* write this operation into the temporal log: the saladmaker starts waiting for the chef */
        if (sem_wait(write_maker_log_sem) < 0) {
            perror("write_maker_log_sem wait failed");
            break;
        }
        fp_log = fopen(temporal_log, "a");
        if (fp_log == NULL){
            printf("File cannot be opened\n");
            exit(-1);
        }
        fprintf(fp_log, "%f: Saladmaker #%d starts waiting for Chef to distribute vegetables\n", (maker_wait_time_start-chef_start_time)/tickspersec, maker_idx);
        fclose(fp_log);
        sem_post(write_maker_log_sem);
        /* wait for the chef_ready semaphore */
        if (sem_wait(chef_ready_sem) < 0) {
            perror("chef_ready_sem wait failed");
            break;
        }
        printf("maker%d: I, maker%d, am ready\n", maker_idx, maker_idx);
        fflush(stdout);
        /* the saladmaker indicates that he is ready */
        sem_post(maker_ready_sem);

        printf("maker%d: wait for you to grab vegetables\n", maker_idx);
        fflush(stdout);
        /* then, the saladmaker will wait for the chef to put vegetables onto the workbench */
        if (sem_wait(put_onto_workbench_sem) < 0) {
            perror("put_onto_workbench_sem wait failed");
            break;
        }
        /* once the above semaphore is posted, the saladmaker's waiting time ends, because he can receive vegetables */
        for (int i = 0; i < 100000000; i++) sum += i;
        maker_wait_time_end = (double) times(&wait_endb);
        printf("maker%d: I can now access the vegetables from workbench, thanks\n", maker_idx);
        fflush(stdout);
        /* write this operation into the temporal log: the saladmaker ends waiting and gets vegetables */
        if (sem_wait(write_maker_log_sem) < 0) {
            perror("write_maker_log_sem wait failed");
            break;
        }
        fp_log = fopen(temporal_log, "a");
        if (fp_log == NULL){
            printf("File cannot be opened\n");
            exit(-1);
        }
        fprintf(fp_log, "%f: Saladmaker #%d ends waiting and starts getting vegetables \n", (maker_wait_time_end-chef_start_time)/tickspersec, maker_idx);
        fclose(fp_log);
        sem_post(write_maker_log_sem);

        /* now, since this saladmaker ends waiting, he needs to go into the parallel_struct to check whether he is in a parallel working status */
        if (sem_wait(check_parallel_status_sem) < 0) {
            perror("check_parallel_status_sem wait failed");
            break;
        }
        /* increment to counter to indicate that he joins the working of saladmakers */
        parallel_struct->maker_counter += 1;
        /* if there are now 2 processes working in parallel, need to start the timer of tracking current parallel working */
        /* if > 2 processes are working in parallel, do not need to start the timer since when there are 2 processes working in parallel, the starting time timestamp is already been stored in parallel_start_time */
        if (parallel_struct->maker_counter == 2){
            /* store as the parallel_start_time in struct, remember to take chef_start_time as the starting point */
            parallel_struct->parallel_start_time = maker_wait_time_end - chef_start_time;
        }
        sem_post(check_parallel_status_sem);

        /* the saladmaker starts preparing and making salad */
        for (int i = 0; i < 100000000; i++) sum += i;
        maker_make_salad_time_start = (double) times(&salad_startb);
        /* first, the saladmaker grabs vegetables from the workbench */
        /* saladmaker #0 receives tomatoes and peppers */
        if (maker_idx == 0){
            tomato_weight = workbench_shm_array[0];
            pepper_weight = workbench_shm_array[1];
            /* if the vegetable weights are valid, accumulate them to the vegetable weight counter */
            if (tomato_weight > 0 && pepper_weight > 0){
                tomato_weight_counter += tomato_weight;
                pepper_weight_counter += pepper_weight;
                onion_weight_counter += 60;
            }
            printf("maker%d: tomato: %d, pepper: %d\n", maker_idx, tomato_weight, pepper_weight);
        }
        /* saladmaker #1 receives tomatoes and onion */
        else if (maker_idx == 1){
            onion_weight = workbench_shm_array[2];
            tomato_weight = workbench_shm_array[3];
            /* if the vegetable weights are valid, accumulate them to the vegetable weight counter */
            if (onion_weight > 0 && tomato_weight > 0){
                onion_weight_counter += onion_weight;
                tomato_weight_counter += tomato_weight;
                pepper_weight_counter += 80;
            }
            printf("maker%d: onion: %d, tomato: %d\n", maker_idx, onion_weight, tomato_weight);
        }
        /* saladmaker #2 receives onions and peppers */
        else {
            onion_weight = workbench_shm_array[4];
            pepper_weight = workbench_shm_array[5];
            /* if the vegetable weights are valid, accumulate them to the vegetable weight counter */
            if (onion_weight > 0 && pepper_weight > 0){
                onion_weight_counter += onion_weight;
                pepper_weight_counter += pepper_weight;
                tomato_weight_counter += 100;
            }
            printf("maker%d: onion: %d, pepper: %d\n", maker_idx, onion_weight, pepper_weight);
        }
        /* the saladmaker then notifies the chef that he finished getting vegetables, and the chef then can take a break */
        sem_post(get_from_workbench_sem);
        /* if the vegetables' weights are not valid, it means that the saladmaker's job has been terminated by the chef, and he can quit */
        if ((maker_idx == 0 && tomato_weight == -1 && pepper_weight == -1) || \
            (maker_idx == 1 && tomato_weight == -1 && onion_weight == -1) || \
            (maker_idx == 2 && onion_weight == -1 && pepper_weight == -1)){
            printf("maker%d: chef notifies me that my job has done!\n", maker_idx);
            printf("maker%d: Let me write my working report into shared memory\n", maker_idx);
            /* before exiting, the saladmaker writes his statistics to the result report shared memory struct */
            if (sem_wait(write_into_report_sem) < 0) {
                perror("write_into_report_sem wait failed");
                break;
            }
            result_struct->salad_count += salad_made;
            result_struct->no_of_salad_made[maker_idx] = salad_made;
            result_struct->ingredients_used[maker_idx*3] = onion_weight_counter;
            result_struct->ingredients_used[maker_idx*3+1] = pepper_weight_counter;
            result_struct->ingredients_used[maker_idx*3+2] = tomato_weight_counter;
            result_struct->salad_making_time[maker_idx] = maker_make_salad_time_accumulator / tickspersec;
            result_struct->waiting_time[maker_idx] = maker_wait_time_accumulator / tickspersec;
            sem_post(write_into_report_sem);

            for (int i = 0; i < 100000000; i++) sum += i;
            timestamp = (double) times(&timestampb);
            /* also, write this operation into temporal log: the saladmaker exited */
            if (sem_wait(write_maker_log_sem) < 0) {
                perror("write_maker_log_sem wait failed");
                break;
            }
            fp_log = fopen(temporal_log, "a");
            if (fp_log == NULL){
                printf("File cannot be opened\n");
                exit(-1);
            }
            fprintf(fp_log, "%f: Saladmaker #%d was notified by Chef and exited\n", (timestamp-chef_start_time)/tickspersec, maker_idx);
            fclose(fp_log);
            sem_post(write_maker_log_sem);
            printf("maker%d: I have finished writing the report, byebye!\n", maker_idx);
            break;
        }

        /* the saladmaker starts to make salad */
        for (int i = 0; i < 100000000; i++) sum += i;
        timestamp = (double) times(&timestampb);
        /* write this operation into the temporal log: the saladmaker starts making salad */
        if (sem_wait(write_maker_log_sem) < 0) {
            perror("write_maker_log_sem wait failed");
            break;
        }
        fp_log = fopen(temporal_log, "a");
        if (fp_log == NULL){
            printf("File cannot be opened\n");
            exit(-1);
        }
        fprintf(fp_log, "%f: Saladmaker #%d starts making salad\n", (timestamp-chef_start_time)/tickspersec, maker_idx);
        fclose(fp_log);
        sem_post(write_maker_log_sem);
        /* randomly generate a salad-making time of saladmaker for this iteration in the given range */
        int make_time = generate_make_salad_time(mkrtime);
        printf("maker%d: I am making the salad ~ It will take about %d seconds\n", maker_idx, make_time);
        /* the saladmaker is making salad in this sleep() */
        sleep(make_time);
        /* increment the no. of salad this maker has made */
        salad_made ++;
        /* end the salad-making time tracker */
        for (int i = 0; i < 100000000; i++) sum += i;
        maker_make_salad_time_end = (double) times(&salad_endb);
        /* one salad is finished, accumulate this round's waiting time and salad making time into the time counter */
        maker_wait_time_accumulator += maker_wait_time_end - maker_wait_time_start;
        maker_make_salad_time_accumulator += maker_make_salad_time_end - maker_make_salad_time_start;

        /* write this operation into the temporal log: the saladmaker finishes making salad */
        if (sem_wait(write_maker_log_sem) < 0) {
            perror("write_maker_log_sem wait failed");
            break;
        }
        fp_log = fopen(temporal_log, "a");
        if (fp_log == NULL){
            printf("File cannot be opened\n");
            exit(-1);
        }
        fprintf(fp_log, "%f: Saladmaker #%d finishes making salad\n", (maker_make_salad_time_end-chef_start_time)/tickspersec, maker_idx);
        fclose(fp_log);
        sem_post(write_maker_log_sem);

        /* the saladmaker will start waiting for the chef in the next iteration again, so before ending this iteration,
        he needs to check whether his exit will lead to the end of parallel working */
        if (sem_wait(check_parallel_status_sem) < 0) {
            perror("check_parallel_status_sem wait failed");
            break;
        }
        /* decrement the #of saladmaker working, becaust this saladmaker is about to exit his active working */
        parallel_struct->maker_counter -= 1;
        /* if the no. of working saladmaker becomes 1, it means that the parallel working period is ending now */
        if (parallel_struct->maker_counter == 1){
            /* note down the ending timestamp, remember to take chef_start_time as starting point */
            double end_timestamp = maker_make_salad_time_end - chef_start_time;
            /* add this period to the total parallel working time accumulator */
            parallel_struct->parallel_time_accumulator += (end_timestamp - (parallel_struct->parallel_start_time)) / tickspersec;
            /* since this parallel working period has ended, write this period into the parallel working time log */
            fp = fopen(parallel_file, "a");
            if (fp == NULL){
                printf("File cannot be opened\n");
                exit(-1);
            }
            /* write the range of time into the file */
            fprintf(fp, "%f ~ %f\n", parallel_struct->parallel_start_time / tickspersec, end_timestamp / tickspersec);
            fclose(fp);
        }
        sem_post(check_parallel_status_sem);
    }

    /* detach the shared memory segment from our process's address space */
    if (shmdt(workbench_shm_array) == -1) {
        perror("shmdt workbench_shm_array: ");
    }
    if (shmdt(result_struct) == -1) {
        perror("shmdt result_struct: ");
    }
    if (shmdt(parallel_struct) == -1) {
        perror("shmdt parallel_shm_struct: ");
    }

    /* the saladmaker closes his semaphore */
    sem_close(chef_ready_sem);
    sem_close(maker_ready_sem);
    sem_close(put_onto_workbench_sem);
    sem_close(get_from_workbench_sem);
    sem_close(write_into_report_sem);
    sem_close(check_parallel_status_sem);
    sem_close(write_maker_log_sem);

    return 0;
}