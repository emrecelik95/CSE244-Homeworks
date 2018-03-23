#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/time.h>

#include "matris.h"
#include "funcs.h"

#define BUFSIZE 1024
#define MILLION 1000000L

unsigned int PORTNUM;


int callSocket(unsigned short);
int readData (int s,void *buf,int n);
void *doClient(void* arg);


typedef struct sizes{
	int row;
	int col;

}m_sizes;


sem_t sem;
m_sizes sizes;

FILE *lastLog;

char lastLogName[BUFSIZE];

long times[1000];
int size = 0;
int clientsNum;
int normalExit = 0;

int ind = 0;

pthread_t *tids;

struct timeval start,end;



void signalHandler(int signo){

	
	gettimeofday(&end, NULL);

	int i;
	double avg = 0, st = 0;
	long timeElapsed;

	for(i = 0 ; i < size ; ++i)
		avg += times[i];
	
	
	avg /= (double)size;	

	double n = (double)size;

	for(i = 0 ; i < n ; ++i)
		st += pow(times[i] - avg,2);

	st /= n;
	st = sqrt(st);

	

	fprintf(lastLog,"The Average Connection Time :  %.2f microseconds\n",avg);
	fflush(lastLog);
	fprintf(lastLog,"The Standard Deviation  :  %.2f\n",st);
	fflush(lastLog);

	if(!normalExit){
		timeElapsed = MILLION*(end.tv_sec - start.tv_sec) +
							end.tv_usec - start.tv_usec;

		fprintf(lastLog,"The time of Termination Signal  :  %.2f\n",st);
		fflush(lastLog);
	}



	fclose(lastLog);

	fprintf(stdout,"Program Terminating...\n");
	exit(0);
	
}



int main(int argc,char** argv){
	
	gettimeofday(&start, NULL);

	if(argc != 5){
		fprintf(stdout,"Usage : <#Port ID >  <#of columns of A> <#of rows of A> <#of clients>\n");
		return 0;
	}

	signal(SIGINT,signalHandler);

	sem_init(&sem,0,1);

	snprintf(lastLogName,BUFSIZE,"logs/clients_%d.log",getpid());
	lastLog = fopen(lastLogName,"w");


	sizes.row = atoi(argv[2]);
	sizes.col = atoi(argv[3]);
	PORTNUM = atoi(argv[1]);

    clientsNum = atoi(argv[4]);
	pthread_t tt[clientsNum];
	tids = tt;


	for(ind = 0 ; ind < clientsNum ; ind++){
		pthread_create(&tids[ind],NULL,doClient,NULL);
	}

	for(ind = 0 ; ind < clientsNum ; ind++){
		pthread_join(tids[ind],NULL);
	}

	while(size < clientsNum);

	normalExit = 1;

	raise(SIGINT);

	return 0;
}




int callSocket(unsigned short portnum)
{
	char hostname[BUFSIZE];

	struct sockaddr_in sa;
	struct hostent *hp;
	int a, sock;
	
	gethostname(hostname, BUFSIZE);
	
	if ((hp = gethostbyname(hostname)) == NULL) {
		errno= ECONNREFUSED;
		return(-1);
	} 
	
	memset(&sa,0,sizeof(sa));
	memcpy((char *)&sa.sin_addr,hp->h_addr,hp->h_length);
	sa.sin_family= hp->h_addrtype;
	sa.sin_port= htons((u_short)portnum);
	
	if ((sock = socket(hp->h_addrtype,SOCK_STREAM,0)) < 0)
		return(-1);
	
	
	if (connect(sock,(struct sockaddr *)&sa,sizeof sa) < 0) {
		close(sock);
		return(-1);
	} 

	return(sock);
}


void *doClient(void* arg){

	sem_wait(&sem);

	int sock;

	
	if((sock = callSocket(PORTNUM)) == -1){
		fprintf(stdout,"Server is not running \n");
		raise(SIGINT);
	}

	sem_post(&sem);
	
	long timedif;
	struct timeval tpend;
	struct timeval tpstart;
	
	gettimeofday(&tpstart, NULL);


	int r = sizes.row;
	int c = sizes.col;
	int i , j ;

	double temp[r*c];

	double **A = allocMatris(r,c);
	double **b = allocMatris(r,1);
	double **x = allocMatris(c,1);
	double e;

	FILE* log;
	char logName[BUFSIZE];
	
	snprintf(logName,BUFSIZE,"logs/client_%d,%ld.log",getpid(),pthread_self());
	log = fopen(logName,"a+");

	//printf("connected socked : %d\n",sock);

	if(writeData(sock,&sizes,sizeof(sizes)) == -1){
		//perror("data writing failed ");
		return 0;
	}


	long int tid;
	if(readData(sock,&tid,sizeof(tid)) == -1){
		//perror("data writing failed ");
		return 0;
	}

	//printf("server tid : %ld\n",tid);


	long int thTid = pthread_self();
	if(writeData(sock,&thTid,sizeof(thTid)) == -1){
		//perror("data writing failed ");
		return 0;
	}
	//printf("thTid : %ld\n",thTid);


	// SERVERDAN VERI ALMA \\
	/////////////////////////

	if(readData(sock,temp,sizeof(temp)) == -1){
		//perror("data writing failed ");
		return 0;
	}


	//printf("Matris received A: \n");
	
	for(i = 0 ; i < r ; i++){
		for(j = 0 ; j < c ; j++)
			A[i][j] = temp[i*c + j];
	}

	//printMatris(A,r,c);

	fprintf(log,"A : ");
	writeMatris(A,r,c,log);
	fflush(log);

	//////////////////////////

	if(readData(sock,temp,sizeof(temp)) == -1){
		//perror("data writing failed ");
		return 0;
	}


	//printf("Matris received b: \n");
	
	for(i = 0 ; i < r ; i++){
		for(j = 0 ; j < 1 ; j++)
			b[i][j] = temp[i*c + j];
	}

	//printMatris(b,r,1);

	fprintf(log,"b : ");
	writeMatris(b,r,1,log);
	fflush(log);

	///////////////////////////

	if(readData(sock,temp,sizeof(temp)) == -1){
		//perror("data writing failed ");
		return 0;
	}


	//printf("Matris received x: \n");
	
	for(i = 0 ; i < c ; i++){
		for(j = 0 ; j < 1 ; j++)
			x[i][j] = temp[i + j];
	}

	//printMatris(x,c,1);

	fprintf(log,"x : ");
	writeMatris(x,c,1,log);
	fflush(log);


	if(readData(sock,&e,sizeof(e)) == -1){
		//("data writing failed ");
		return 0;
	}

	fprintf(log,"e : %f\n",e);
	fflush(log);

	//printf("error received e: %f\n",e);

	freeMatris(A,r);
	freeMatris(b,r);
	freeMatris(x,c);

	fclose(log);


	gettimeofday(&tpend, NULL);

	timedif = MILLION*(tpend.tv_sec - tpstart.tv_sec) +
					   tpend.tv_usec - tpstart.tv_usec;


	sem_wait(&sem);
	times[size] = timedif;
	++size;
	sem_post(&sem);

}