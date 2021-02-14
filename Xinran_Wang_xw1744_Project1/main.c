#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "voter.h"
#include "read_data.h"

int people_in_hash = 0;
int people_voted = 0;
int table_size;

/* main function for execution */
int main(int argc, char** argv){
	Hash_and_ll hash_table_and_ll;
    char* file_address;
	char command[1024];
    char **elements;
    int counter;

    /* get the file address and hash table size from flags */
    for (int c = 1; c < argc; c += 2){
        if (strcmp(argv[c], "-f") == 0){
            file_address = argv[c+1];
        }
        if (strcmp(argv[c], "-m") == 0){
            table_size = atoi(argv[c+1]);
        }
	}
    printf("Data Address: %s\n", file_address);
    printf("Hash Table size: %d\n", table_size);

    /* create hash table using the given table size */
    hash_table_and_ll.voter = (Voter*) malloc(table_size * sizeof(Voter));
	hash_table_and_ll.voter = init_hash_table(hash_table_and_ll.voter);
	printf("Successfully created hash table...\n");

    /* read the csv data from the given address */
    hash_table_and_ll.voter = read_csv(file_address, hash_table_and_ll.voter);
	printf("Successfully read data into hash table...\n");

    /* create zip code linked list */
    hash_table_and_ll.postcode = (Postcode*) malloc(sizeof(Postcode));
	hash_table_and_ll.postcode->next = NULL;
	printf("Successfully created voter linked list...\n");

	/* the main loop of receiving user command input */
    while(1){
		printf("\n##### Welcome to Vote Monitoring System #####\n");
		printf("i <rin> <first name> <last name> <zip code> - insert a new person into system;\n");
		printf("l <rin> - lookup a person's info;\n");
		printf("d <rin> - delete a person from system;\n");
		printf("r <rin> - register this person as voted;\n");
		printf("bv <file address> - bulk vote for all rin's in this file;\n");
		printf("z <zip code> - list the voting info for this zip code area;\n");
		printf("ph - show the hash table of all persons in the system;\n");
		printf("pl - show the linked list ordered by zip code;\n");
		printf("v - show the number of voted people;\n");
		printf("perc - show the percentage of voted people over all people in the system;\n");
		printf("o - show the list of zipcodes in descending order of the number of voted people;\n");
		printf("exit - exit the system.\n");	
		printf("##########\n");
		printf("\nYour command: ");

		scanf(" %1023[^\n]%*c", command);
		/* split the command line by whitespace */
		counter = split(command, ' ', &elements);

		if (counter == 5){
			/* if there are 5 arguments, it is the insertion command */
			if (strcmp(elements[0], "i") == 0){
				hash_table_and_ll.voter = insert(hash_table_and_ll.voter, elements[2], elements[3], atoi(elements[1]), atoi(elements[4]));
			} else{
				printf("Wrong input format, please check again.\n");
			}
		} else if (counter == 2){
			if (strcmp(elements[0], "l") == 0){
				/* lookup a person's info */
				search(hash_table_and_ll.voter, atoi(elements[1]));
			} else if (strcmp(elements[0], "d") == 0){
				/* delete a person from system */
				hash_table_and_ll = delete(hash_table_and_ll.voter, hash_table_and_ll.postcode, atoi(elements[1]));
			} else if (strcmp(elements[0], "r") == 0){
				/* register a person as voted */
				hash_table_and_ll = regist(hash_table_and_ll.voter, hash_table_and_ll.postcode, atoi(elements[1]));
			} else if (strcmp(elements[0], "bv") == 0){
				/* bulk vote: be careful that the address cannot include whitespace! */
				hash_table_and_ll = bulk_vote(hash_table_and_ll.voter, hash_table_and_ll.postcode, elements[1]);
			} else if (strcmp(elements[0], "z") == 0){
				/* list voting info of this zip code */
				print_by_postcode(hash_table_and_ll.postcode, atoi(elements[1]));
			} else{
				printf("Wrong input format, please check again.\n");
			}
		} else if (counter == 1){
			if (strcmp(elements[0], "ph") == 0){
				/* pring hash table */
				print_hash_table(hash_table_and_ll.voter);
			} else if (strcmp(elements[0], "pl") == 0){
				/* print linked list */
				print_ll(hash_table_and_ll.postcode);
			} else if (strcmp(elements[0], "v") == 0){
				/* print no. of voted people */
				printf("The number of voted people: %d\n", people_voted);
			} else if (strcmp(elements[0], "perc") == 0){
				/* print percentage of voted people */
				printf("There is/are %d people in the system, %d of which have voted.\n", people_in_hash, people_voted);
				double percentage = (people_voted * 100.0 / people_in_hash);
				printf("The voted percentage is %.2f %%.\n", percentage);
			} else if (strcmp(elements[0], "o") == 0){
				/* print descending order of linked list */
				printf("##### Descending No. of voter by zipcode #####\n");
				hash_table_and_ll.postcode = sort_ll(hash_table_and_ll.postcode);
				print_ll(hash_table_and_ll.postcode);
			} else if (strcmp(elements[0], "exit") == 0){
				/* free the memory in hash table */
				hash_table_and_ll.postcode = free_ll_element(hash_table_and_ll.postcode);
				free(hash_table_and_ll.postcode);
				hash_table_and_ll.postcode = NULL;
				printf("Successfully released Linked List memory...\n");
				/* free the memory in linked list */
				hash_table_and_ll.voter = free_hash_table_element(hash_table_and_ll.voter);
				free(hash_table_and_ll.voter);
				hash_table_and_ll.voter = NULL;
				printf("Successfully released Hash Table memory...\n");
				printf("You have Exited the System!\n");
				break;
			} else{
				printf("Wrong input format, please check again.\n");
			}
		} else{
			printf("Wrong input format, please check again.\n");
		}
		/* free the memory of tokens */
        for (int y = 0; y < counter; y++) free (elements[y]);
        free (elements);
	}
    return 0;
}
