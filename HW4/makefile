all: main

main: main.o
	gcc main.o -pthread -o grepTh
    
main.o: main.c
	gcc -c main.c -pthread
    
clean: 
	rm *.o grepTh
