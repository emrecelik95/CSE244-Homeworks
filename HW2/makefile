all: main

main: main.o
	gcc main.o -o listdir
    
main.o: main.c
	gcc -c main.c
    
clean: 
	rm *.o listdir
