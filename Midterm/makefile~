all: server client results

server: server.o
	gcc -o timerServer server.o matris.c -lm
    
server.o: server.c
	gcc -c server.c 
	
client: client.o
	gcc -o seeWhat client.o matris.c -lm
	
client.o: client.c
	gcc -c client.c 
	
results: results.o
	gcc -o showResults results.o matris.c -lm
	
results.o: results.c
	gcc -c results.c 

clean: 
	rm *.o timerServer seeWhat showResults
