#target: dependencies
#	action

all: chef saladmaker

chef: chef.o chef_help.o
	gcc chef.o chef_help.o -o chef -lpthread

chef.o: chef.c chef_help.h
	gcc -c chef.c

chef_help.o: chef_help.c chef_help.h
	gcc -c chef_help.c

saladmaker: saladmaker.o saladmaker_help.o
	gcc saladmaker.o saladmaker_help.o -o saladmaker -lpthread

saladmaker.o: saladmaker.c saladmaker_help.h chef_help.h
	gcc -c saladmaker.c

saladmaker_help.o: saladmaker_help.c saladmaker_help.h
	gcc -c saladmaker_help.c

clean:
	rm *.o chef saladmaker *.txt