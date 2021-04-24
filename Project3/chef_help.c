#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <time.h>
#include "chef_help.h"

/* given the cheftime as input, generate the actual break time for the chef in the range of:
[0.5 * cheftime, cheftime] */
int generate_chef_break_time(int cheftime){
    srand(time(NULL));
    int lower = 0.5 * cheftime;
    int result = (rand() % (cheftime - lower + 1)) + lower;
    // if the rest time is 0, turn it into 1 to give the chef a break time
    if (result == 0) return 1;
    return result;
}

/* the chef grabs a tomato, whose weight ranges from 0.8 * 100g to 1.2 * 100g = [80, 120]g */
int grab_tomato(){
    srand(time(NULL));
    int tomato_low = 80;
    int tomato_hi = 120;
    int tomato = (rand() % (tomato_hi - tomato_low + 1)) + tomato_low;
    return tomato;
}

/* the chef grabs a pepper, whose weight ranges from 0.8 * 80g to 1.2 * 80g = [64, 96]g */
int grab_pepper(){
    srand(time(NULL));
    int pepper_low = 64;
    int pepper_hi = 96;
    int pepper = (rand() % (pepper_hi - pepper_low + 1)) + pepper_low;
    return pepper;
}

/* the chef grabs an onion, whose weight ranges from 0.8 * 60g to 1.2 * 60g = [48, 72]g */
int grab_onion(){
    srand(time(NULL));
    int onion_low = 48;
    int onion_hi = 72;
    int onion = (rand() % (onion_hi - onion_low + 1)) + onion_low;
    return onion;
}

/* this function helps the chef to print out the results written in the result_report shared memory */
void print_results_shm(result_report *report){
    printf("---------- Results and Statistics ----------\n");
    printf("Total No. of salads produced: %d\n", report->salad_count);
    for (int m = 0; m < 3; m++){
        printf("Saladmaker #%d statistics:\n", m);
        printf("No. of salads made: %d\n", report->no_of_salad_made[m]);
        printf("Total time working on salad making: %f seconds\n", report->salad_making_time[m]);
        printf("Total time spent on waiting for chef's vegetables: %f seconds\n", report->waiting_time[m]);
        printf("Amount of vegetables used: %d grams of onion, %d grams of pepper, %d grams of tomato\n", report->ingredients_used[3*m], report->ingredients_used[3*m+1], report->ingredients_used[3*m+2]);
    }
    printf("---------- End of Report ----------\n");
}
