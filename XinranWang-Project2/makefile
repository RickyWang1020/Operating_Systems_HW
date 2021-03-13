#target: dependencies
#	action

all: myhie bubble_sort merge_sort

myhie: main.o read_file.o node.o sorter.o
	gcc main.o read_file.o node.o sorter.o -o myhie

main.o: main.c read_file.h node.h sorter.h
	gcc -c main.c

bubble_sort: bubble_sort.o
	gcc bubble_sort.o -o bubble_sort

bubble_sort.o: bubble_sort.c
	gcc -c bubble_sort.c

merge_sort: merge_sort.o
	gcc merge_sort.o -o merge_sort

merge_sort.o: merge_sort.c
	gcc -c merge_sort.c

read_file.o: read_file.c read_file.h
	gcc -c read_file.c

node.o: node.c node.h sorter.h read_file.h
	gcc -c node.c

sorter.o: sorter.c sorter.h read_file.h
	gcc -c sorter.c

clean:
	rm *.o *.csv myhie bubble_sort merge_sort worker*