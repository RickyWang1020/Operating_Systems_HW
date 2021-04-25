# Programming Assignment 3

## About

A multi-process program involving semaphores and shared memory written by Xinran Wang (xw1744) for Programming assignment 3 of CS-UH 3010 Operating Systems Spring 2021.

## Design Ideas

1. I use named semaphores and shared memory blocks to ensure the inter-process communications between chef and saladmaker, and named semaphores are also used as protections when multiple saladmakers attempt to write into log files.

2. I set the rule that saladmaker #0 only has onions, saladmaker #1 only has peppers, saladmaker #2 only has tomatoes. So that the when the chef picks tomatoes and peppers, she will deliver to saladmaker #0, when chef picks tomatoes and onions, she will deliver to saladmaker #1, when chef picks onions and peppers, she will deliver to saladmaker #2.

3. The chef needs to grab 2 vegetables at a time. In order to make this random selection simplier, I choose to let the chef program generate one random integer in [0, 2] for each vegetable-picking process, which means that the chef will be delivering vegetable to the saladmaker corresponding to this random integer, and the chef will be picking the two vegetables that this saladmaker lacks.

4. The resting time of chef and the salad-making time of saladmakers are designed to be integers, since `sleep()` is used to simulate the periods.

5. When the No. of salad is satisfied, the chef will start to notify all saladmakers to quit. The method I implement is that the chef "sends" vegetables of weight `-1` to all saladmakers, so that saladmakers will know they are asked to leave their work.

## Sketch of IPC Structures

- Between Chef and Saladmaker #i (i is 0 or 1 or 2):
  - `chef_ready[i]` (initialized to 0) semaphore array: indicate whether chef is ready to send vegetable to saladmaker #i
  - `maker_ready[i]` (initialized to 0) semaphore array: indicate whether saladmaker #i is ready for communicating with the chef
  - `put_onto_workbench` (initialized to 0) semaphore: used to protect `workbench_shm_arr` for the chef to put (write) vegetables' weight values into it
  - `get_from_workbench` (initialized to 0) semaphore: used to protect `workbench_shm_arr` for the saladmaker #i to receive (read) vegetables' weights

``` c
// chef workflow
V(chef_ready[i])
P(maker_ready[i])
// chef starts to pick and put vegetables to workbench_shm_arr
V(put_onto_workbench)
P(get_from_workbench)
// chef takes a break
```

``` c
// saladmaker #i workflow
P(chef_ready[i])
V(maker_ready[i])
P(put_onto_workbench)
V(get_from_workbench)
// saladmaker picks vegetables from workbench_shm_arr and starts making salad
```

- Among Saladmaker processes:

  - `write_into_report` (initialized to 1) semaphore is used to protect `result_struct` shared memory struct, so that only one saladmaker can write into it at one time
  - `write_maker_log` (initialized to 1) semaphore is used to protect `temporal_log.txt`, so that only one saladmaker can write into it at one time
  - `check_parallel_status` (initialized to 1) semaphore is used to protect `parallel_time.txt` and `parallel_struct`, so that every time only one saladmaker can enter and check whether he is in a parallel working status, so as to assist the recording of parallel working period

## Program Invocation

1. Run `makefile` to create `chef` and `saladmaker` file.
2. Run `./chef` with two flags (the order of the flags does not matter)
    - `"-n"` flag means the number of salad you want the saladmakers to produce.
    - `"-m"` flag means the maximum "break" time that the chef will take after she finishes each iteration of delivering vegetables to saladmakers.
3. The `chef` program will **automatically** invoke the `saladmaker` program as its child processes by `exec()` using the following flags:
    - `"-m"`, the maximum salad-making time of this saladmaker.
    - `"-t"`, the timestamp of chef's starting time, which is used to derive the relative timestamp of saladmakers (so as to write cleanly into time logs).
    - `"-i"`, the index of current saladmaker (0 or 1 or 2).
    - `"-w"`, the shared memory id of workbench array, which is used for chef to send vegetables and saladmakers to receive vegetables.
    - `"-r"`, the shared memory id of result report struct, which is used for saladmakers to report their total waiting time, total salad-making time, total vegetables used and No. of salads made.
    - `"-p"`, the shared memory id of the struct used to record parallel working, which is for tracking the parallel working time periods of saladmakers.

4. At the end of the program, it will first report the *total number of salads produced*. Then, it will report individual working result of every saladmaker: *total time spent on salad-making*, *total time waiting for delivery of vegetables* from chef, *sum weights of each vegetable* that the saladmaker has used. Also, the program will generate **two** log files, one records the *time intervals and total amout of time that saladmakers worked in parallel* (`parallel_time.txt`), the other records the timeline operation of each saladmaker (`temporal_log.txt`).

## Other References

1. Semaphore cleanup in Mac: <https://stackoverflow.com/questions/62174130/semaphore-cleanup-in-linux-c>; <https://stackoverflow.com/questions/2143404/delete-all-system-v-shared-memory-and-semaphores-on-unix-like-systems>

2. Semaphore not working on Mac: <https://stackoverflow.com/questions/26797126/why-sem-wait-doesnt-wait-semaphore-on-mac-osx>

3. Scanf to array: <https://stackoverflow.com/questions/16299727/c-scanf-to-array>

4. Shm int array: <https://stackoverflow.com/questions/21227270/read-write-integer-array-into-shared-memory>

5. Shm struct: <https://stackoverflow.com/questions/34534683/shared-memory-with-array-of-structs>
