#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#include "matris.h"
#include <sys/time.h>
#define MILLION 1000000L

#define FIFO_PERM (S_IRUSR | S_IWUSR)
#define BUF_MAX 255

#define SIZE_MAX 4096

struct matris_t{
	int n;
	double matris[20][20];
};


static volatile sig_atomic_t doneflag = 0;

long int clients[SIZE_MAX];
int size = 0;

char* mainfifo = NULL;
int fds = 0;
FILE* file = NULL; 
static long int* all = NULL;
int capacity = 10;
int allIndex = 0;

static void signalHandler(int signo,siginfo_t* siginfo ,void *context){
	if(signo == SIGINT){
		doneflag = 1;
		fprintf(stdout,"Program terminating...\n");

		int i;
		for(i = 0 ; i < allIndex ; ++i){
			kill(all[i],SIGTERM);
			char buf[BUF_MAX];
			snprintf(buf,BUF_MAX,"%ld",all[i]);
			unlink(buf);
		}

		close(fds);
		unlink(mainfifo);
		fprintf(file,"---------->Ctrl+C Signal received . Program terminating<----------");
		fflush(file);
		fclose(file);
		free(all);
		exit(0);
	}
	else if(signo == SIGUSR1){
		//fprintf(stderr," Client pid : %ld \n",(long)(siginfo->si_pid));

		int i,found = 0;;
		for(i = 0 ; i < size ; ++i)
			if(clients[i] == siginfo->si_pid)
				found = 1;

		if(!found){
			clients[size++] = siginfo->si_pid;
			//fprintf(stderr,"eklendi : %d \n",siginfo->si_pid);
		}		

		found = 0;
		for(i = 0 ; i < allIndex ; ++i)
				if(all[i] == siginfo->si_pid)
					found = 1;

		if(!found){
			if(allIndex >= capacity){
				capacity *= 2;
				all = (long int*)realloc(all,capacity);
			}
			all[allIndex++] = siginfo->si_pid;
		}
		
	}

}


int main(int argc , char** argv){
	
	if(argc != 4){
		fprintf(stderr,"Usage : <ticks in miliseconds>  <n>  <mainpipename>\n");
		return 0;
	}

	all = (long int*)malloc(sizeof(long int)*capacity);

	long ticks;
	int n;

	char logName[BUF_MAX];

	struct sigaction actRec;

	ticks = atoi(argv[1]);
	n = atoi(argv[2]) * 2;
	mainfifo = argv[3];
	long int serverPid = getpid();	
	long int clientPid;			
	strcpy(logName ,"log/timerServer.log");
	file = fopen(logName,"w");

	long timedif;
	struct timeval tpre;
	struct timeval tcurrent;
	struct timeval tStart;


	if (gettimeofday(&tpre, NULL)) {
		fprintf(stderr, "Failed to get start time\n");
		raise(SIGINT);
	}

	tStart = tpre; // baslangıc zamanını al

	pid_t mainPid , timePid;
	////////////////////////////////////// SIGNAL
	actRec.sa_flags = SA_SIGINFO;
	actRec.sa_sigaction = &signalHandler;

	if(sigemptyset(&actRec.sa_mask) == -1 || sigaction(SIGUSR1,&actRec,NULL) == -1   
										  || sigaction(SIGINT ,&actRec,NULL) == -1
										  || sigaction(SIGTERM ,&actRec,NULL) == -1){ 
		fprintf(stderr,"signal error\n");
	}
	//////////////////////////////////////

	//fprintf(stdout,"Server pid : %ld\n",serverPid);
	//fprintf(stdout,"Waiting for a signal\n");

	if((mainPid = fork()) < 0){
		perror("Fork failed");
		raise(SIGINT);
	}
	if(mainPid > 0){
		while(1){

			if (gettimeofday(&tcurrent, NULL)) {
				fprintf(stderr, "Failed to get end time\n");
				raise(SIGINT);
			}
			timedif = MILLION*(tcurrent.tv_sec - tpre.tv_sec) +	tcurrent.tv_usec - tpre.tv_usec;

			if(timedif / 1000 >= ticks && size > 0){
				tpre = tcurrent;

				//printf("topladı\n");
				//printf("size : %d \n",size);

				
				if((timePid = fork()) < 0){
					perror("Failed to fork");
					raise(SIGINT);
				}
				if(timePid == 0){
					int index = 0;

					while(index < size){	

						clientPid = clients[index];
						pid_t pid;
						//fprintf(stdout,"Client requested : %ld\n",clientPid);

						
						if((pid = fork()) < 0){
							perror("Fork failed");
							raise(SIGINT);
						}
						if(pid == 0){
							int fdClient = -1;
							char fifoC[BUF_MAX];
							struct timeval tGen;
							long int genTime;

							snprintf(fifoC,BUF_MAX,"%ld",clientPid);

							if((mkfifo(fifoC,FIFO_PERM)) == -1){
								perror("Failed creating fifo");
								raise(SIGINT);
							}
							
							double** tmp = generateMatris(n);
							if (gettimeofday(&tGen, NULL)) {
								fprintf(stderr, "Failed to get start time\n");
								raise(SIGINT);
							}
							genTime = MILLION*(tGen.tv_sec - tStart.tv_sec) +
											tGen.tv_usec - tStart.tv_usec;

							struct matris_t ma;
							int i ,j;
							memset(&ma,0,sizeof(ma));

							for(i = 0 ; i < n ; ++i)
								for(j = 0 ; j < n ; ++j)
									ma.matris[i][j] = tmp[i][j];

							ma.n = n;
							

							while (((fdClient = open(fifoC,O_WRONLY)) == -1) && (errno == EINTR));
							write(fdClient,&ma,sizeof(struct matris_t));

							close(fdClient);
							unlink(fifoC);
							
							fprintf(file,"ClientPid: %ld , ",clientPid);
							fprintf(file,"Time : %.5f ms, ",(double)genTime/1000);
							fprintf(file,"Determinant : %.1f",det(tmp,n));
							fprintf(file,"\n");

							fflush(file);
							freeMatris(tmp,n);

							//printf("exitt\n");
							exit(0);
						}
						if(pid > 0){
							//wait(NULL);
							++index;
							if(doneflag)
								exit(0);
						}
						
					}	
					exit(0);
				}
				if(timePid > 0){
					size = 0;
					if(doneflag)
						exit(0);
				}

			
			}
		}
		
	}

	if(mainPid == 0){

		while(1)
		{

			if (mkfifo(mainfifo, FIFO_PERM) == -1){
				if (errno != EEXIST) { 				
					fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",
						(long)getpid(), argv[1], strerror(errno));
					return 1;
				}
				perror("Failed to create fifo");
				return 0;
			}
			while (((fds = open(mainfifo,O_WRONLY)) == -1));
			write(fds,&serverPid,sizeof(serverPid));
			close(fds);
			unlink(mainfifo);
			
			if(doneflag)
				exit(0);
		}

	}
	
	return 0;	
}
