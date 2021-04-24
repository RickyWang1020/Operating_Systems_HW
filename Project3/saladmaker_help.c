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
#include "saladmaker_help.h"

/* given the salmkrtime as input, generate the actual salad-making time for the salad maker in the range of:
[0.8 * salmkrtime, salmkrtime] */
int generate_make_salad_time(int salmkrtime){
    srand(time(NULL));
    int lower = 0.8 * salmkrtime;
    int result = (rand() % (salmkrtime - lower + 1)) + lower;
    // the saladmaker has to take some time to make salad!
    if (result == 0) return 1;
    return result;
}
