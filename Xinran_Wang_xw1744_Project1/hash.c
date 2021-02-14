#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "voter.h"

/* initialize hash table by assigning each slot's head to NULL */
Voter *init_hash_table(Voter *table){
    for(int i = 0; i < table_size; i++){
    	table[i].next = NULL;
    }
	return table;
}

/* print hash table */
void print_hash_table(Voter *table){
    int i;
    Voter *current;

	printf("##### Hash Table with All Voters' RINs #####\n");
	
    for (i = 0; i < table_size; i++){
		/* if this slot is empty, only print the empty slot */
		if (table[i].next == NULL){
			printf("Slot[%d] -- \n", i);
			continue;
		}
		/* if this slot is not empty, traverse and print all values */
		current = table[i].next;
		printf("Slot[%d]", i);
		while (current != NULL){
			printf(" -> %d",current->RIN);
			current = current->next;
		}
		printf("\n");	
	}
	printf("##### End of Hash Table #####\n");
}

/* insert a new value to hash table */
Voter *insert(Voter *table, char *first, char *last, int rin, int zip){
    Voter *new_voter, *current;
	int hash_index = h(rin, table_size);

	/* create a new voter struct with the information */
    new_voter = (Voter*)malloc(sizeof(Voter));
	strcpy(new_voter->first, first);
	strcpy(new_voter->last, last);
    new_voter->RIN = rin;
	new_voter->zip_code = zip;
	new_voter->voted = 0;
    new_voter->next = NULL;

	/* insert the new voter into hash table's linked list
	if "to insert" is the first one in this slot, just add it to this slot */
	if(table[hash_index].next == NULL){
		table[hash_index].next = new_voter;
		people_in_hash ++;
		return table;
    }

	/* otherwise, iterate to the end of this slot's linked list to insert */
    current = table[hash_index].next;
    while(current->next != NULL){
		/* if rin has overlap, abort the insertion */
		if (current->RIN == rin){
			printf("People with RIN %d already exists, cannot insert.\n", rin);
			return table;
		}
		current = current->next;
    }
    current->next = new_voter;
	people_in_hash ++;
	printf("Successfully inserted person with RIN %d.\n", rin);
    return table;
}

/* search for the info of a person by RIN */
void search(Voter *table, int rin){
    int hash_index = h(rin, table_size);
    Voter *current;

    current = table[hash_index].next;

	/* if the slot is empty, report not found */
	if(current == NULL){
		printf("Voter with RIN %d is not found.\n", rin);
        return;
    }

	/* otherwise, loop through the linked list in the current hashtable index */
    while(current != NULL){
		if(current->RIN == rin){
			printf("Voter with RIN %d is found:\n", rin);
			printf("First name: %s\n", current->first);
			printf("Last name: %s\n", current->last);
			printf("Zip code: %d\n", current->zip_code);
			if (current->voted) printf("Vote status: Voted\n");
			else printf("Vote status: Not voted\n");
			return;
		}
		current = current->next;
    }
    printf("Voter with RIN %d is not found.\n", rin);
}

/* free all linked list elements in hash table */
Voter *free_hash_table_element(Voter *table){
	Voter *cur;
	/* iterate through every hash table slot */
	for (int t = 0; t < table_size; t++){
		/* free up every linked list element's memory */
		while (table[t].next != NULL){
			cur = table[t].next;
			table[t].next = table[t].next->next;
			free(cur);
			cur = NULL;
		}
	}
	return table;
}

