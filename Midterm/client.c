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

#define MILLION 1000000L
#define FIFO_PERM (S_IRUSR | S_IWUSR)
#define BUF_MAX 255


struct matris_t{
	int n;
	double matris[20][20];
};

static int count = 0;
FILE* tempFile;
FILE* file;
char tempLog[BUF_MAX];
long int serverPid = -1;
int n;

static volatile sig_atomic_t doneflag = 0;

static void signalHandler(int signo,siginfo_t* siginfo ,void *context){
	if(signo == SIGINT){
		
		unlink(tempLog);

		fprintf(stdout,"Program terminating...\n");
		if(file != NULL){
			fprintf(file,"---------->Ctrl+C Signal received . Program terminating<----------");
			fflush(file);
			fclose(file);
		}
		kill(serverPid,SIGINT);	
		doneflag = 1;
	}
	else if(signo == SIGTERM){
		doneflag = 1;
		fprintf(stdout,"Program terminating...\n");
		if(tempFile != NULL){
			fclose(tempFile);
			unlink(tempLog);
		}
		if(file != NULL)
			fclose(file);

		unlink(tempLog);
		exit(0);
	}

}


int main(int argc , char** argv){
	
	if(argc != 2){
		printf("Usage : ./seeWhat <mainpipename>\n");
		return 0;
	}

	char* mainfifo = argv[1];
	char strMat[BUF_MAX];
	
	int fds;
	long int clientPid = getpid();
	char strPid[BUF_MAX];
	
	pid_t pid1,pid2;
	
	long timedif;
	struct timeval tpend;
	struct timeval tpstart;

	struct matris_t mat;

	struct sigaction act;


	snprintf(strPid,BUF_MAX,"%ld",clientPid);
	char log[BUF_MAX] = "log";
	char logName[BUF_MAX];

	strcpy(tempLog,"log/tempShow.log");///////
	tempFile = fopen(tempLog,"w+");/////////////////////////

	strcat(log,strPid);


	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = &signalHandler;

	if(sigemptyset(&act.sa_mask) == -1 || sigaction(SIGUSR1,&act,NULL) == -1   // 
								       || sigaction(SIGINT ,&act,NULL) == -1
								       || sigaction(SIGTERM,&act,NULL) == -1){ // CTRL - C
		fprintf(stderr,"signal error\n");
	}	

	fds = -1;
	while(fds == -1)
		while (((fds = open(mainfifo,O_RDONLY)) == -1) && (errno == EINTR)) ;
	
	
	while(serverPid == -1)
		read(fds,&serverPid,sizeof(serverPid));
	close(fds);	
	
	//fprintf(stdout,"serverPid : %ld\n",serverPid);

	while(1){
		
		usleep(200);

		++count;
		strcpy(logName,"log/");
		strcat(logName,"log_");
		strcat(logName,strPid);
		strcat(logName,"_");
		char strCount[BUF_MAX];
		snprintf(strCount,BUF_MAX,"%d",count);
		strcat(logName,strCount);
		int fd[2];

		if(kill(serverPid,SIGUSR1) == -1)
			perror("Failed to send the SIGUSR1 signal\n");
		/*else
			fprintf(stdout,"Signal sent to %ld\n",serverPid);*/


		//fprintf(stdout,"This client pid : %ld\n",clientPid);
																							
		
		fds = -1;
		while(fds == -1)
			while (((fds = open(strPid, O_RDONLY)) == -1) && (errno == EINTR));
		

		memset(&mat,0,sizeof(mat));

		while(read(fds,&mat,sizeof(struct matris_t)) == -1);
		close(fds);

		n = mat.n;
		double** matris = allocMatris(n);
		copyMatris(matris,mat.matris,n);

		file = fopen(logName,"a+");
		fprintf(file,"Normal Matris = ");
		writeMatris(matris,n,file);
		fflush(file);

		freeMatris(matris,n);////////////////////
		fclose(file);
		//printMatris(matris,n);///////////////////////////////
		//printf("\n");////////////////////////
		

		if (pipe(fd) == -1) { 
			perror("Failed to create the pipe");
			raise(SIGINT);
		}

		if((pid1 = fork()) == -1){
			perror("Fork failed ");
			raise(SIGINT);
		}
		if(pid1 == 0){
			double** shifted = allocMatris(n);
			double** matris2 = allocMatris(n);
			copyMatris(matris2,mat.matris,n);
			shiftedInverse(matris2,shifted,n);

			if (gettimeofday(&tpstart, NULL)) {
				fprintf(stderr, "Failed to get start time\n");
				raise(SIGINT);
			}

			double result1 = det(matris2,n) - det(shifted,n);	printf("res1 : %f\n",result1);

			if (gettimeofday(&tpend, NULL)) {
				fprintf(stderr, "Failed to get end time\n");
				raise(SIGINT);
			}
			timedif = MILLION*(tpend.tv_sec - tpstart.tv_sec) +
			tpend.tv_usec - tpstart.tv_usec;
			double time1 = (double)timedif/1000.0;
													
			file = fopen(logName,"a+");
			fprintf(file,"ShiftedInverse Matris = ");
			writeMatris(shifted,n,file);
			fflush(file);

			freeMatris(shifted,n);
			freeMatris(matris2,n);
			fclose(file);
 			//fclose(tempFile);

			//printMatris(shifted,n);//////////////
			//printf("\n");////////////////////////


			close(fd[0]);
         	write(fd[1],&result1,sizeof(result1));
         	write(fd[1],&time1,sizeof(time1));
         	close(fd[1]);


			exit(0);
		}
		if(pid1 > 0){

			wait(NULL);

			
			double result1;
	    	double time1;

			close(fd[1]);
	    	read(fd[0],&result1,sizeof(result1)); printf("res1 gelen: %f\n",result1);
	    	read(fd[0],&time1,sizeof(time1));
	    	close(fd[0]);

			if((pid2 = fork()) == -1){
				perror("Fork failed ");
				raise(SIGINT);
			}

			if(pid2 == 0){
				double** conv = allocMatris(n);
				double** kernel = allocMatris(n);
				double result2;

				int i,j;
				for(i = 0 ; i < 3 ; i++){
					for(j = 0 ; j < 3 ; j++){
						if((i==1 && j == 1)) 
							kernel[i][j] = 1;
						else
							kernel[i][j] = 0;
					}
				}

				double** matris3 = allocMatris(n);
				copyMatris(matris3,mat.matris,n);

				convolution(matris3,n,kernel,3,conv);

				

				if (gettimeofday(&tpstart, NULL)) {
					fprintf(stderr, "Failed to get start time\n");
					raise(SIGINT);
				}

				result2 = det(matris3,n) - det(conv,n);		

				if (gettimeofday(&tpend, NULL)) {
					fprintf(stderr, "Failed to get end time\n");
					raise(SIGINT);
				}
				timedif = MILLION*(tpend.tv_sec - tpstart.tv_sec) +
									tpend.tv_usec - tpstart.tv_usec;
				double time2 = (double)timedif/1000.0;
					
				
				file = fopen(logName,"a+");											
				fprintf(file,"Convolution Matris = ");
				writeMatris(conv,n,file);
				fflush(file);
				fclose(file);

				freeMatris(conv,n);
				freeMatris(kernel,3);
				freeMatris(matris3,n);
				//printMatris(conv,n);//////////////////////
				//printf("\n");////////////////////////

		    	//printf("time1 : %.4f  , time2 : %.6f\n",time1,time2);
		    	//printf("res1 : %.4f  , res2 : %.6f\n",result1,result2);
		    	char out[BUF_MAX];
		    	char strRes1[BUF_MAX],strRes2[BUF_MAX],strTime1[BUF_MAX],strTime2[BUF_MAX],strCount[BUF_MAX];
		    	snprintf(strRes1,BUF_MAX,"%.4f",result1);
		    	snprintf(strRes2,BUF_MAX,"%.4f",result2);
		    	snprintf(strTime1,BUF_MAX,"%.5f",time1);
		    	snprintf(strTime2,BUF_MAX,"%.5f",time2);
		    	snprintf(strCount,BUF_MAX,"%d",count);

		    	strcpy(out,strPid);
		    	strcat(out," ");
		    	strcat(out,strCount);
		    	strcat(out," ");
		    	strcat(out,strRes1);
		    	strcat(out," ");
		    	strcat(out,strTime1);
		    	strcat(out," ");
		    	strcat(out,strRes2);
		    	strcat(out," ");
		    	strcat(out,strTime2);
		    	strcat(out,"\n");

		    	fprintf(tempFile,"%s",out);

				//fclose(tempFile);////////////

				exit(0);
			}

			if(pid2 > 0){
				wait(NULL);
				if(doneflag)
					exit(0);
			}
	
		}

		while(wait(NULL) != -1);

		if(doneflag)
			exit(0);

	}
	return 0;	

}
