#include <stdio.h>
#include <stdlib.h>
#include "hash.h"
#include "voter.h"

/* mark a person as voted in hash table and add this person to linked list */
struct Hash_and_ll regist(Voter *table, Postcode *post_ll, int rin){
    int index = h(rin, table_size);
    Voter *current;
    struct Hash_and_ll to_return;

    to_return.voter = table;
    to_return.postcode = post_ll;

    current = table[index].next;

	/* if the hashtable index is empty, abort */
	if(current == NULL){
		printf("Voter with RIN %d is not found, cannot change register status.\n", rin);
        return to_return;
    }

	/* otherwise, loop through the linked list in current hashtable index and change vote status */
    while(current != NULL){
		if(current->RIN == rin){
			if (current->voted){
                printf("Voter with RIN %d has already voted.\n", rin);
            } else{
                // printf("RIN: %d, code: %d\n", current->RIN, current->zip_code);
                current->voted = 1;
                post_ll = insert_voted_to_ll(post_ll, current);
                people_voted ++;
                // print_ll(post_ll);
                to_return.postcode = post_ll;
                printf("Changed voter with RIN %d to 'voted'.\n", rin);
            }
			return to_return;
		}
		current = current->next;
    }
    /* if did not found such RIN, abort */
    printf("Voter with RIN %d is not found, cannot change register status.\n", rin);
    return to_return;
}

/* a helper function for regist() to add the people to linked list */
Postcode *insert_voted_to_ll(Postcode *post_ll, Voter *vote_people){
    Postcode *post_cur, *post_slot;
    Voter_holder *voter_cur, *voter_slot;

    post_cur = post_ll;

    /* loop through every existing zip code in linked list to find the matched one */
    while (post_cur->next != NULL){
        post_cur = post_cur->next;
        /* if find the matched one */
        if (post_cur->code == vote_people->zip_code){
            
            voter_cur = post_cur->voter_holder;
            /* increment the number of voter in this zip code area */
            post_cur->num_of_voter ++;
            /* create a pointer pointing to Voter struct */
            voter_slot = (Voter_holder*)malloc(sizeof(Voter_holder));
            voter_slot->voter = vote_people;
            voter_slot->next = NULL;
            /* loop to the end of linked list and insert the pointer */
            while (voter_cur->next != NULL)
                voter_cur = voter_cur->next;
            voter_cur->next = voter_slot;
            return post_ll;
        }
    }

    /* if there is no such zip code, create a new zip code slot */
    Voter_holder *empty_voter;
    empty_voter = (Voter_holder*)malloc(sizeof(Voter_holder));
    voter_slot = (Voter_holder*)malloc(sizeof(Voter_holder));
    voter_slot->voter = vote_people;
    voter_slot->next = NULL;
    empty_voter->next = voter_slot;

    post_slot = (Postcode*) malloc(sizeof(Postcode));
    post_slot->code = vote_people->zip_code;
    post_slot->num_of_voter = 1;
    post_slot->next = NULL;
    post_slot->voter_holder = empty_voter;
    /* add the new zip code slot to the end of the linked list */
    post_cur->next = post_slot;
    // print_ll(post_ll);
    return post_ll;
}

/* delete a person from the system */
struct Hash_and_ll delete(Voter *table, Postcode *post_ll, int rin){
    int index = h(rin, table_size);
    Voter *prev, *person_to_delete;
    struct Hash_and_ll to_return;
	
    to_return.voter = table;
    to_return.postcode = post_ll;

    /* if the person is the first node in the hash table's current link */
    if(table[index].next->RIN == rin){
        /* if this person has voted, also delete it from zipcode linked list */
        if (table[index].next->voted){
            // printf("%d\n", table[index].next->RIN);
            post_ll = delete_voted_from_ll(post_ll, table[index].next);
            to_return.postcode = post_ll;
        }
        /* delete the people from hash table */
        person_to_delete = table[index].next;
        // printf("%d\n",person_to_delete->RIN);
		table[index].next = table[index].next->next;
        /* free the allocated memory */
        free(person_to_delete);
        person_to_delete = NULL;
        people_in_hash --;
		printf("Voter with RIN %d has been successfully deleted.\n", rin);
        to_return.voter = table;
		return to_return;
    }

    prev = table[index].next;
		
    /* if the hash table's current link is empty, abort */
    if(prev == NULL){
		printf("Voter with RIN %d is not found, cannot delete.\n", rin);
		return to_return;
    }
    /* otherwise, loop through the current link to find the wanted person */
    while(prev->next != NULL){
		if(prev->next->RIN == rin){
            /* if the person has voted, also delete it from zipcode linked list */
            if (prev->next->voted){
                post_ll = delete_voted_from_ll(post_ll, prev->next);
                to_return.postcode = post_ll;
            }
            /* delete the people from hash table */
            person_to_delete = prev->next;
            // printf("%d\n",person_to_delete->RIN);
			prev->next = prev->next->next;
            /* free the allocated memory */
            free(person_to_delete);
            person_to_delete = NULL;
            people_in_hash --;
			printf("Voter with RIN %d has been successfully deleted.\n", rin);
			to_return.voter = table;
		    return to_return;
		}
	    prev = prev->next;
    }
    /* if loop to the end and still did not find the person, abort */
    printf("Voter with RIN %d is not found, cannot delete.\n", rin);
    return to_return;
}

/* a helper function for delete() to delete the people from linked list */
Postcode *delete_voted_from_ll(Postcode *post_ll, Voter *vote_people){
    Postcode *post_prev, *post_to_del;
    Voter_holder *voter_prev, *voter_to_del;

    post_prev = post_ll;

    /* traverse the linked list to find the matched zip code node */
    while (post_prev->next != NULL){
        if (post_prev->next->code == vote_people->zip_code){
            
            voter_prev = post_prev->next->voter_holder;
            post_prev->next->num_of_voter --;

            /* if the linked list zip code node has 0 voters, delete this node */
            if (post_prev->next->num_of_voter == 0){
                voter_to_del = voter_prev->next;
                post_to_del = post_prev->next;
                post_prev->next = post_prev->next->next;
                /* free the allocated memory */
                free(voter_to_del);
                free(post_to_del);
                voter_to_del = NULL;
                post_to_del = NULL;
                // printf("%d\n",voter_to_del->voter->RIN);
                // printf("%d\n",post_to_del->num_of_voter);
                people_voted --;
                return post_ll;
            }
            /* otherwise, traverse in the current zip code area to find the matched person */
            while (voter_prev->next != NULL){
                if (voter_prev->next->voter->RIN == vote_people->RIN){
                    voter_to_del = voter_prev->next;
                    voter_prev->next = voter_prev->next->next;
                    /* free the allocated memory */
                    free(voter_to_del);
                    voter_to_del = NULL;
                    // printf("%d\n",voter_to_del->voter->RIN);
                    people_voted --;
                    return post_ll;
                }
                voter_prev = voter_prev->next;
            }
        }
        post_prev = post_prev->next;
    }
    /* after traversal, we didn't find the matched zip code (it is impossible!) */
    printf("No matched zip code in LL!\n");
    return post_ll;
}

/* print all voted voters in a given zip code area */
void print_by_postcode(Postcode *post_ll, int zip){
    Postcode *post_cur;
    Voter_holder *voter_cur;

    post_cur = post_ll->next;
    while (post_cur != NULL){
        if (post_cur->code == zip){
            printf("Number of voted people in zip-code %d area: %d\n", post_cur->code, post_cur->num_of_voter);
            voter_cur = post_cur->voter_holder->next;
            while (voter_cur != NULL){
                printf("---Voter RIN: %d\n", voter_cur->voter->RIN);
                voter_cur = voter_cur->next;
            }
            return;
        }
        post_cur = post_cur->next;
    }
    printf("There are no voted people in zip-code %d area.\n", zip);
    return;
}

/* print the whole zip code linked list as well as all the voted people in each zip code area */
void print_ll(Postcode *post_ll){
    Postcode *post_cur;
    Voter_holder *voter_cur;

    post_cur = post_ll->next;

    /* if linked list is empty */
    if (post_cur == NULL){
        printf("##### Linked List is Empty #####\n");
        return;
    }

    /* otherwise, traverse and print */
    printf("##### Linked List of Voted People #####\n");
    while (post_cur != NULL){
        printf("Post code: %d, number of voted people: %d\n", post_cur->code, post_cur->num_of_voter);
        voter_cur = post_cur->voter_holder->next;
        while (voter_cur != NULL){
            printf("---Voter RIN: %d\n", voter_cur->voter->RIN);
            voter_cur = voter_cur->next;
        }
        post_cur = post_cur->next;
    }
    printf("##### End of Linked List #####\n");
    return;
}

/* use bubble sort to sort the zip code linked list in descending order */
Postcode *sort_ll(Postcode *post_ll){
    int length = 0;
    Postcode *cur, *temp1, *temp2, *to_move;

    /* get the length of linked list */
    cur = post_ll;
    while (cur->next != NULL){
        cur = cur->next;
        length ++;
    }

    for(int i = length-2; i >= 0; i--){
		temp1 = post_ll;
		temp2 = temp1->next;
        /* compare every pair of nodes */
		for(int j = 0; j <= i; j++){
            // printf("%d, %d\n", temp1->next->code, temp2->next->code);
            // printf("%d, %d\n", temp1->next->num_of_voter, temp2->next->num_of_voter);
			if(temp1->next->num_of_voter < temp2->next->num_of_voter){
                /* swapping the two nodes */
				to_move = temp1->next;
				temp1->next = temp2->next;
                to_move->next = temp1->next->next;
				temp1->next->next = to_move;
                temp1 = temp1->next;
			} else{
                /* else, proceed */
                temp1 = temp1->next;
			    temp2 = temp2->next;
            }
		}
	}
    return post_ll;
}

/* free all elements in zipcode linked list */
Postcode *free_ll_element(Postcode *post_ll){
    Postcode *post_head, *post_cur;
    Voter_holder *voter_head, *voter_cur;

    post_head = post_ll;

    // printf("RIN: %d, code: %d\n", vote_people->RIN, vote_people->zip_code);
    // printf("%d\n", vote_people->RIN);
    /* loop through every zip code slot */
    while (post_head->next != NULL){
        // printf("zip: %d\n", post_head->next->code);
        voter_head = post_head->next->voter_holder;
        /* free up every voter pointer slot's memory */
        while (voter_head->next != NULL){
            // printf("rin: %d\n", voter_head->next->voter->RIN);
            voter_cur = voter_head->next;
            voter_head->next = voter_head->next->next;
            // printf("rin: %d\n", voter_cur->voter->RIN);
            free(voter_cur);
            voter_cur = NULL;
        }
        /* free up the zip code slot's memory */
        post_cur = post_head->next;
        post_head->next = post_head->next->next;
        free(post_cur);
        post_cur = NULL;
    }
    // print_ll(post_ll);
    return post_ll;
}