# Programming Assignment 2

## About

A multi-process sorting program written by Xinran Wang (xw1744) for Programming assignment 2 of CS-UH 3010 Operating Systems Spring 2021.

## Design Ideas

1. The structure of this program: Root node is the `main.c` file, it uses `fork()` to create a Coordinator node as its child. The Coordinator node will `fork()` many Sorter nodes as its children, and when all the Sorters ended their tasks and exited, the Merger node will appear in the code area of Coordinator, and starts to read sorted results reported by each Sorter. The Merger then merges many sorted results into a whole sorted list, and write the sorted data lines into the output file. Finally the Coordinator/ Merger node exits to the Root node, and the Root node terminates the whole program.

2. The No. of Sorters is somehow restricted: if too many Sorters are working at the same time, there will be too many opened fifos, leading to errors, so I **limit the Max number of Sorters to be 2200**. Also, compared with the number of data lines, if there are too few Sorters present, the number of data passed into the `exec` file will be too large, leading to "exec error: too long argument list", so I also **limit the average Max lines of data for a Sorter to be 1500**. These restrictions can help this program function better.

3. The two sorting algorithms I implement are `bubble sort` and `merge sort`. A Sorter with odd index will use bubble sort, and a Sorter with even index will use merge sort.

4. In this program, the No. of `SIGUSR1` received by the Root node is usually smaller than the actual No. of Sorters. This is mainly because of "race condition", where many asynchronous processes (in my program they are nearly completed sorting algorithm processes) send signals to another process (in my program it is the Root) nearly at the same time, and the signal handler function is not fully set up before it can catch the next signal. So that some sent signals are not "seen" because the signal handler is busy handling another incoming signal and is not ready when these "unlucky" signals come in.

## File Structure

- `main.c`: the file representing the Root node
  - Global variables:
    - `no_of_sig1`: a counter to count the number of `SIGUSR1` caught by the Root node.
    - `no_of_sig2`: a counter to count the number of `SIGUSR2` caught by the Root node.
  - `int main(int argc, char** argv)`: first receives in-line parameters as flags. Then the Root node (this program) creates the Coordinator node as its child. When the Coordinator finishes its work, the Root node will report the No. of `SIGUSR1` and `SIGUSR2` it received. The Root also tracks the **turnaround time for this whole sorting program**, and prints it out when the program is about to exit.
- `bubble_sort.c` and `merge_sort.c`: the two sorting functions served as `exec` files for the sorter to pass data and perform sorting. It receives the parsed numeric data within current sorter's working ranges, performs sorting (Referred from: <https://www.geeksforgeeks.org/iterative-merge-sort/>), and uses a named pipe (each Sorter has a named pipe of its own: for instance, sorter #5 will write into a fifo named "worker5") to pass the sorted data's *index* into the Merger node one by one. It keeps track of the working time, and passes the time it spent to perform sorting into the Merger node also by the named pipe. Finally, it sends `SIGUSR1` to the Root node, releases all the allocated memory, and exits.
- `read_file.c` and `read_file.h`: some useful functions for reading data
  - `void alloc_mem_for_str(char** arr, int position, char* txt, int mem_size)`: helps to allocate memory for a string, and put the string into the given position of an array of strings (Referred from: <https://stackoverflow.com/questions/8600181/allocate-memory-and-save-string-in-c>).
  - `int get_no_of_lines(char* filename)`: iterate through the given file and get the No. of lines of this file.
  - `void separate_line(char *filename, char *output_arr[], int line_no)`: given the file, parse out every line of data, and put every line as one entry into an array of strings (Referred from: <https://stackoverflow.com/questions/50310178/split-string-on-any-number-of-white-spaces-in-c/50310606>), so that we can access each line by its line number index of the array.
  - `void parse_wanted_num(char *data_line_arr[], double *output_arr, int attribute_no, int list_length)`: for every line of data, split it by spaces and get the attribute data (given by index 0 or 3 or 4 or 5) we want to perform sort on, and store this numeric data into a double-type array (because the income is represented in float number, and it is ok if we want to store id/ dependents/ zip-code into the double-type array), so that sorters do not need to access the full string data, but just work on parts of a numeric array.
- `node.c` and `node.h`: the program of Coordinator's work
  - `void get_sort_range(int* start_from, int* sort_ranges, int no_of_worker, int random_range, int no_of_lines)`: given the No. of sorter, the total No. of data lines, and whether to use random range, this function will generate sorting ranges for each sorter, and store each sorter's sort starting index and No. of data lines to sort separately in the corresponding position of two arrays (for instance, worker #0 will have its starting point and no. of lines to sort in `start_from[0]` and `sort_ranges[0]`).
  - `void create_coord(char* file_address, int no_of_worker, int random_range, int attribute_number, int order, char* output_address, int no_of_lines, pid_t root_id)`: this function will first pre-process the data file: get the sorting ranges for each worker, parse every line of data into an array of strings, put the wanted numeric data for sorting in a double-type array, and initialize the fifo for every Sorter. Then it creates a multiple of Sorter nodes (Referred from: <https://stackoverflow.com/questions/876605/multiple-child-process>) where each sorter will enter its working function defined in `sorter.c`.
  
    After that, this program enters the Merger node mode, where it will be responsible for waiting for each Sorter to return. If all the Sorters have exited, the Merger will use `poll()` to pull the parts of sorted number's indices from every fifo written by each Sorter one by one (Referred from: <https://stackoverflow.com/questions/24922069/how-to-use-poll-when-dealing-with-multiple-file-descriptors>, <https://stackoverflow.com/questions/22021253/poll-on-named-pipe-returns-with-pollhup-constantly-and-immediately>), and use a `int**` array to store the lists of sorted numbers' indices for every Sorter's fifo (for example, Sorter #3's array of data will be stored in the array's [3] position, where an `int*` pointer points to the list of elements written by Sorter #3). Also, it receives the Sorter's runtime data as the final line of data read from every fifo, and stores this number in an array of double number, so that it can report the runtime of Sorters at the end of program.

    Next, it uses the `merge_data()` function to merge a multiple of sorted indices' arrays into one long index array, where each number entry in the index array means where should the corresponding index of the original data lines be in the final sorted result (for example, if we sort in ascending order, and the first number in the merged index array is 5, it means the 6-th line in the original data lines has the smallest numeric value). Then the Merger will loop through the merged index array, and access the corresponding line of data, and write that line into the output file.

    Finally, the Merger sends a `SIGUSR2` to the Root, and when it is caught, it basically marks the end of Coordinator/ Merger's work. The Merger tracks its own runtime for merging those index lists, and will report it on ttys after printing out all Sorters' runtime. Before exiting, it will release all allocated memory.
  - `void merge_data(int *merged_index, double *number_data, int **index_lists, int no_of_worker, int no_of_lines, int *sort_ranges, int order)`: this function is responsible for merging a multiple of sorted index arrays into one large merged index array, where each index indicates the corresponding data line's position in the sorted numeric field. It assigns a position pointer to the head of each index array at first, then in each iteration we compare the numeric value associated with each position pointer, and pick out the largest (if sorted in descending) or smallest (if sorted in ascending) value among them and put the corresponding index in the merged index array, and move this position pointer forward. Repeat this process until every position pointer reaches the end of their index arrays, and we will get a big merged index array.
  - `void sig2_handler(int signum)`: the handler function for the Root to deal with `SIGUSR2`, i.e., increment the No. of `SIGUSR2` received.
- `sorter.c` and `sorter.h`: some functions related to Sorter's work
  - `void prepare_and_sort(double *all_data_for_read, int sorter_idx, int start, int range, char **my_data, int order, int root_id)`: this is the working area for one Sorter node. It prepares the array of data to execute the sorting `exec` file: the Sorter's index is needed to open the corresponding fifo in the sorting algorithm, the Sorter's sorting range is required to serve as the "length" of numeric array to sort, the sorting order is needed to remind the sorting algorithm of the order, the root node's ID is also needed so as to send signal to it, and parts of the numeric data (parsed by `parse_wanted_num`) will then be appended to the array. Then, it will call the `execvp()` function to perform sorting and provide its part of sorted results.
  - `void init_fd_for_worker(struct pollfd *pollfds, int worker)`: it initializes all the file descriptors that will be used by Sorters and will be read by Merger. First it creates the fifo name: "worker" + the index of Sorter, then it opens the fifo and puts it into an array of file descriptors for future polling (Referred from: <https://stackoverflow.com/questions/15055065/o-rdwr-on-named-pipes-with-poll>).
  - `void sig1_handler(int signum)`: the handler function for the Root to deal with `SIGUSR1`, i.e., increment the No. of `SIGUSR1` received.

## Program Invocation

1. Run `makefile` to create `myhie`, `bubble_sort` and `merge_sort` file.
2. Run `./myhie` with a multiple of flags (the order of the flags does not matter)
    - `"-i"` flag means the path of the input `.csv` file (be careful not to include any whitespaces in the path!).
    - `"-k"` flag means the No. of sorters you want to do the sort work. The program will report error if invalid number is given (larger than the No. of data lines, or smaller than 0). Moreover, to make sure that the fifo and merging can function smoothly, too many sorters (> 2200) and too few sorters (# of data lines / # of sorters > 1500) will also be rejected by this program.
    - `"-a"` flag means the attribute index to perform sorting. The program will report error if numbers other than 0, 3, 4, 5 are given.
    - `"-o"` flag means the order of sorting. "d" is descending, "a" is ascending. The program will report error if other characters are given.
    - `"-s"` flag means the path and name of the output file to store the sorted data. If such a file does not exist .originally, the program will create one; if such a file exists, the program will **overwrite** on this file.
    - `"-r"` flag is an *optional* flag, if included, every sorter will have a random No. of data lines to work on; if not included, every sorter will sort a same No. of data lines.
3. The program will report the real runtime for each sorter to complete its work, the real runtime for the merger node to complete its work, the total turnaround time of this program, and the number of `SIGUSR1` and `SIGUSR2` received by the Root node.

## Other References

1. fd debug: <https://stackoverflow.com/questions/11258781/bad-file-descriptor-with-linux-socket-write-bad-file-descriptor-c>
2. new line character: <https://stackoverflow.com/questions/28429625/check-if-string-contains-new-line-character>
3. dup2: <https://stackoverflow.com/questions/11042218/c-restore-stdout-to-terminal>
