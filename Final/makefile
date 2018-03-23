all: server clients

server: server.o
	gcc server.o funcs.c matris.c -o server -pthread -lm
	
clients: clients.o
	gcc clients.o funcs.c matris.c -o clients -pthread -lm
    
server.o: server.c
	gcc -c server.c
    
clients.o: clients.c
	gcc -c clients.c
    
clean: 
	rm *.o server clients
