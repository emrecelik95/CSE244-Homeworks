all: main

main: main.o
	gcc main.o -pthread -o grepSh
    
main.o: main.c
	gcc -c main.c -pthread
    
clean: 
	rm *.o grepSh
