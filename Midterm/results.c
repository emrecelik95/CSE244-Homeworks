#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "matris.h"
#include <string.h>

#define BUF_MAX 255

long int clientPid;
FILE* results;
char* fname = "log/tempShow.log";

static void signalHandler(int signo,siginfo_t* siginfo ,void *context){
	if(signo == SIGINT){
		fprintf(stdout,"Program terminating...\n");
		fprintf(results,"---------->Ctrl+C Signal received . Program terminating<----------");
		fflush(results);
		kill(clientPid,SIGINT);
		unlink(fname);
		exit(0);
	}
	if(signo == SIGTERM){
		fprintf(stdout,"Program terminating...\n");
		exit(0);
	}

}

int main(void){

	
	FILE* file = fopen(fname,"r");
	results = fopen("log/showResults.log","w");
	long int curbyte = 0;

	if(file == NULL){
		fprintf(stderr,"there is no client \n");
		exit(0);
	}
	char buf[BUF_MAX];
	int clCount;
	double res1,res2,time1,time2;
	
	struct sigaction act;
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &signalHandler;

	if(sigemptyset(&act.sa_mask) == -1 || sigaction(SIGUSR1,&act,NULL) == -1   
								 	   || sigaction(SIGINT ,&act,NULL) == -1
								   	   || sigaction(SIGTERM ,&act,NULL) == -1){ 
		fprintf(stderr,"signal error\n");
	}

	while(1){
		if(file == NULL)
			raise(SIGTERM);

		fseek(file,curbyte ,SEEK_SET); 
		if(fgets(buf,BUF_MAX,file) != NULL){
			sscanf(buf,"%ld %d %lf %lf %lf %lf",&clientPid,&clCount,&res1,&time1,&res2,&time2);
			printf("Pid : %ld\tResult1 : %.4f\tResult2 : %.4f\n",clientPid,res1,res2);
			fprintf(results,"Pid of client : %ld\n",clientPid);
			fprintf(results,"Result 1 : %lf\tTime elapsed : %lf\n",res1,time1);
			fprintf(results,"Result 2 : %lf\tTime elapsed : %lf\n\n",res2,time2);
			fflush(results);

		}
		curbyte = ftell(file);
		fclose(file);
		file = fopen(fname,"r");
	}

	fclose(file);
	fclose(results);
	return 0;
}
