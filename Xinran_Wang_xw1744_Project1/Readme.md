# Programming Assignment 1

## About

A vote monitoring system written by Xinran Wang (xw1744) for Programming assignment 1 of CS-UH 3010 Operating Systems Spring 2021.

## File Structure

- `main.c`: the main C file for program execution
  - Global variables:
    - `people_in_hash`: an integer tracking the number of people in the system (i.e., loaded into the hash table)
    - `people_voted`: an integer tracking the number of people voted
    - `table_size`: an integer representing the size of hash table, will be received from user's "-m" flag
  - `int main(int argc, char** argv)`: the main function of this program. It first receives the `.csv` file's absolute path and the size of hash table as two flags, loads the data, and creates hash table and zipcode linked list. Then it will continuously ask for user command to execute.
- `voter.c` and `voter.h`: define basic structs of the person and functions about insertion, deletion and printing in hash table and linked list
  - Struct type definitions:
    - `struct Voter`: holds the information of a person: RIN, first name, last name, zip code, whether voted, and a pointer pointing to the next element in hash table.
    - `struct Voter_holder`: a linked list to hold a voter in the current zipcode element, it includes a pointer pointing to `Voter` struct, and a pointer pointing to the next `Voter_holder` slot in the current zipcode area.
    - `struct Postcode`: the zipcode linked list element, it includes the current zipcode, current number of voter in this area, the `Voter_holder` linked list, and a pointer pointing to the next element.
    - `struct Hash_and_ll`: it holds both the hash table and zipcode linked list, for the sake of some functions' multiple return value (Referred from <https://stackoverflow.com/questions/2620146/how-do-i-return-multiple-values-from-a-function-in-c>).
  - `struct Hash_and_ll regist(Voter *table, Postcode *post_ll, int rin)`: mark a person as "voted" in hash table, and also add the person to the zipcode linked list.
  - `Postcode *insert_voted_to_ll(Postcode *post_ll, Voter *vote_people)`: a helper function for `regist()` to add the person into the linked list.
  - `struct Hash_and_ll delete(Voter *table, Postcode *post_ll, int rin)`: delete a person from hash table, and if the person has voted, also delete it from linked list.
  - `Postcode *delete_voted_from_ll(Postcode *post_ll, Voter *vote_people)`: a helper function for `delete()` to delete the person from linked list.
  - `void print_by_postcode(Postcode *post_ll, int zip)`: given a zipcode, print the corresponding area's number of voted people and their RINs.
  - `void print_ll(Postcode *post_ll)`: print the whole zipcode linked list.
  - `Postcode *sort_ll(Postcode *post_ll)`: use bubble sort to sort the zipcode linked list by descending number of voted people (Referred from: <https://www.w3resource.com/c-programming-exercises/linked_list/c-linked_list-exercise-30.php>).
  - `Postcode *free_ll_element(Postcode *post_ll)`: free the memory of all elements in the linked list so as to exit the system.

- `hash.c` and `hash.h`: define insertion, printing and queries in hash table
  - The hash function defined by this program is `hash_index = num % table_size`
  - `Voter *init_hash_table(Voter *table)`: initialize the hash table by setting every slot's head node to NULL.
  - `void print_hash_table(Voter *table)`: print the whole hash table.
  - `Voter *insert(Voter *table, char *first, char *last, int rin, int zip)`: given information, insert a new person into the hash table.
  - `void search(Voter *table, int rin)`: given RIN, search for this person in the system and print the person's information.
  - `Voter *free_hash_table_element(Voter *table)`: free the memory of all elements in the hash table so as to exit the system.

- `read_data.c` and `read_data.h`: hold functions about receiving and parsing inputs
  - `int split (const char *txt, char delim, char ***tokens)`: given a string, parse it by the given delimiter, store the parsed string tokens in pointer `tokens` and return the number of tokens (Referred from: <https://stackoverflow.com/questions/9210528/split-string-with-delimiters-in-c>).
  - `Voter *read_csv(char *filename, Voter *data)`: given the absolute path of the `.csv` data file, read the lines of data and insert the data into hash table (Referred from: <https://stackoverflow.com/questions/45449905/use-strtok-read-csv-file>).
  - `struct Hash_and_ll bulk_vote(Voter *table, Postcode *post_ll, char* address)`: given the directory of the text file including the RINs of people wanted to be voted **(one RIN on a line)**, mark these people as "voted" and add them to linked list.

## Program Invocation

1. Run `makefile` to create `mvote` file
2. Run `./mvote` with 2 flags. "-m" flag means the hash table size, "-f" flag means the absolute path of the `.csv` data file to read. Examples:

    ```./mvote -m 31 -f /absolute-path/voters50.csv```

    or

    ```./mvote -f /absolute-path/voters50.csv -m 31```

3. Type command line in the console as instructed in the program. Basic functions of the programs are as follows:
    - `i <rin> <first name> <last name> <zip code>`: insert a new person into system. It will abort if there exists a person with the same RIN.
    - `l <rin>`: lookup a person's info by the given RIN. It will abort if such person doesn't exist.
    - `d <rin>`: delete a person from system. It will abort if such person doesn't exist.
    - `r <rin>`: register this person as voted. It will abort if such person doesn't exist or this person has already voted.
    - `bv <file address>`: bulk vote for all RIN's included in the given file.
    - `z <zip code>`: list the number of people voted and registered person in the given zip code area.
    - `ph`: *A useful method added by myself.* Show the hash table of all persons in the system.
    - `pl`: *A useful method added by myself.* Show the linked list ordered by zip code.
    - `v`: show the number of voted people.
    - `perc`: show the percentage of voted people over all people in the system.
    - `o`: show the list of zipcodes in descending order of the number of voted people.
    - `exit`: *release all the allocated memory* and exit the system.

## Other References

1. Receive input: <https://stackoverflow.com/questions/40949545/file-path-as-user-input-in-c>
2. Scanf not waiting for user input: <https://stackoverflow.com/questions/18372421/scanf-is-not-waiting-for-user-input>
3. Free a pointer: <https://stackoverflow.com/questions/8300853/how-to-check-if-a-pointer-is-freed-already-in-c/8300887#:~:text=You%20have%20to%20do%20your%20own%20bookkeeping.&text=There%20is%20no%20reliable%20way,if%20a%20pointer%20is%20freed>
