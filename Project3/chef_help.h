#ifndef CHEF_HELP_H
#define CHEF_HELP_H

#ifndef STRUCT_RESULT_REPORT
#define STRUCT_RESULT_REPORT
/* the struct for the total result, which will be attached to the result shared memory */
typedef struct Result_Report{
    /* the total no. of salads produced */
    int salad_count;
    /* the no. of salads made by each saladmaker */
    int no_of_salad_made[3];
    /* the time accumulator of salad making time for each saladmaker */
    double salad_making_time[3];
    /* the time accumulator of waiting for vegetables for each saladmaker */
    double waiting_time[3];
    /* the vegetable weight accumulator array for saladmakers */
    /* the first three slots will put the weight of used onion, pepper and tomato of maker0 */
    /* the next three slots will put the weight of used onion, pepper and tomato of maker1 */
    /* the final three slots will put the weight of used onion, pepper and tomato of maker2 */
    int ingredients_used[9];
} result_report;
#endif

#ifndef STRUCT_PARALLEL_WORKING
#define STRUCT_PARALLEL_WORKING
/* the struct for record parallel working time, which will be attached to the parallel_work shared memory */
typedef struct Parallel_Working{
    /* this variable keeps track of the #of makers currently working on salad */
    int maker_counter;
    /* this varlable is an accumulator of recording the parallel working time */
    double parallel_time_accumulator;
    /* this variable stores the starting timestamp that at least 2 makers start working in parallel, 
    so that when #of makers are working at the same time decreases to <=1, we can take a timestamp and subtract this timestamp variable */
    double parallel_start_time;
} parallel_working;
#endif

int generate_chef_break_time(int cheftime);
int grab_tomato();
int grab_pepper();
int grab_onion();
void print_results_shm(result_report *report);

#endif
