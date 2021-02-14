#target: dependencies
#	action

mvote: main.o hash.o read_data.o voter.o
	gcc main.o hash.o read_data.o voter.o -o mvote

main.o: main.c hash.h voter.h read_data.h
	gcc -c main.c

hash.o: hash.c hash.h voter.h
	gcc -c hash.c

read_data.o: read_data.c read_data.h hash.h voter.h
	gcc -c read_data.c

voter.o: voter.c hash.h voter.h
	gcc -c voter.c

clean:
	rm *.o mvote
