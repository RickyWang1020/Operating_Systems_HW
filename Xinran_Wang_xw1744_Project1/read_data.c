#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "voter.h"
#include "read_data.h"
#include "hash.h"

/* split a string by given delimiter and return the No. of separated tokens in this string 
also stores the parsed string tokens into the tokens pointer variable */
int split (const char *txt, char delim, char ***tokens){
    int *tklen, *t, count = 1;
    char **arr, *p = (char *) txt;

    /* when encountering the delimiter, increment the counter */
    while (*p != '\0') if (*p++ == delim) count += 1;
    /* store each token's length in an array */
    t = tklen = calloc (count, sizeof (int));
    for (p = (char *) txt; *p != '\0'; p++) *p == delim ? *t++ : (*t)++;
    /* allocate space for token pointer */
    *tokens = arr = malloc (count * sizeof (char *));
    t = tklen;
    p = *arr++ = calloc (*(t++) + 1, sizeof (char *));
    /* store each token in the array of tokens */
    while (*txt != '\0'){
        if (*txt == delim){
            p = *arr++ = calloc (*(t++) + 1, sizeof (char *));
            txt++;
        }
        else *p++ = *txt++;
    }
    free (tklen);
    return count;
}

/* read data in csv and insert each line of data into the system */
Voter *read_csv(char *filename, Voter *data){
	FILE *file;
    file = fopen(filename, "r");

    // printf("Opened file\n");
    char line[2000];

	while (fgets(line, sizeof(line), file)){
        char **element_token;
        const char *line_token;
        int count;

        /* split out one line by "," */
        line_token = strtok(line, ",");
        // printf("%s\n", line_token);
        /* parse the line by white space */
        count = split(line_token, ' ', &element_token);

        line_token = strtok(NULL, ", ");
        /* insert the data of a new person into hash table */
        data = insert(data, element_token[1], element_token[2], atoi(element_token[0]), atoi(element_token[3]));

        /* free the memory of tokens */
        for (int x = 0; x < count; x++) free (element_token[x]);
        free (element_token);
    }
    return data;
}

/* given the absolute path of the file containing RINSs of people to vote, bulk vote them */
struct Hash_and_ll bulk_vote(Voter *table, Postcode *post_ll, char* address){
    FILE *file;
    char line[100];
    int cur_rin;
    struct Hash_and_ll to_return;

    to_return.voter = table;
    to_return.postcode = post_ll;
    
    file = fopen(address, "r");
    if (file == NULL){
        printf("File is empty, cannot bulk vote.\n");
        return to_return;
    }
    /* one RIN per line! */
    while (fgets(line, sizeof(line), file)) {
        cur_rin = atoi(line);
        to_return = regist(to_return.voter, to_return.postcode, cur_rin);
    }
    return to_return;
}
