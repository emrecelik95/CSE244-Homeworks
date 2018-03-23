#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "matris.h"
#include "funcs.h"

#define BUFSIZE 4096

#define SVD 0
#define QR 1
#define PSDINV 2

typedef struct sizes{
	int row;
	int col;

}m_sizes;


typedef struct solver{
	m_sizes sizes;
	int method;
	double** A;
	double** b;
	int key;
	pthread_mutex_t shMutex;
}sArgs;

int establish(unsigned int);
int getConnection(int);
void *doForClient(void* arg); 
void *doForClientPool(void* arg);
void *solve(void* arg);

void doMatrixProcesses(m_sizes sizes,int sock,long int tid);


unsigned int PORTNUM;

sem_t sem;
int sock;

int current = 0;
pthread_t *tidsG;
int size = 0;

void signalHandler(int signo){
	/*int i;
	for(i = 0 ; i < size ; i++)
		pthread_join(tidsG[i],NULL);
*/

	close(sock);
	current = 0;
	
	exit(0);

}

void childHandler(int signo){

	int shmid;

	shmid = shmget(getppid(),sizeof(double),IPC_CREAT | 0666);
	shmctl(shmid, IPC_RMID, NULL);

	shmid = shmget(getpid(),sizeof(double),IPC_CREAT | 0666);
	shmctl(shmid, IPC_RMID, NULL);


	exit(0);

}


int main(int argc,char** argv){
	
	if(argc != 3){
		fprintf(stdout,"Usage : <port #, id> <thpool size, k >\n");
		return 0;
	}

	signal(SIGINT,signalHandler);

	sem_init(&sem,0,1);

	int s,cli,thPool = atoi(argv[2]);


	PORTNUM = atoi(argv[1]);


	if((sock = establish(PORTNUM)) == -1){
		perror("sock error ");
		return 0;
	}

	if(thPool == 0){ // per request
		pthread_t tids[1500];
		size = 1500;
		tidsG = tids;
		int i = 0;

		for(;;){
			sem_wait(&sem);
			if ((s = accept(sock,NULL,NULL)) < 0){		
				perror("get connection error ");
				return 0;
			}

			pthread_create(&tids[i++],NULL,doForClient,&s);
			
		}



	}
	else{ // thread pool
		pthread_t tids[thPool];
		size = thPool;
		tidsG = tids;
		int i;
		for(i = 0 ; i < thPool ; i++){
			pthread_create(&tids[i],NULL,doForClientPool,NULL);
		}

		for(;;);
	}


	close(sock);////////////////////////////////////////////

	return 0;
}

int establish(unsigned int portnum){
	char hostname[BUFSIZE];
	int sock;

	struct sockaddr_in sa;
	struct hostent *hp;

	memset(&sa, 0, sizeof(struct sockaddr_in));
	
	gethostname(hostname, BUFSIZE);
	hp = gethostbyname(hostname);
	if (hp == NULL) 
		return(-1);

	printf("Host Name : %s , Port ID : %d\n",hostname,portnum);//////////////////////////

	sa.sin_family = hp->h_addrtype;
	sa.sin_port = htons(portnum);


	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	if (bind(sock,(struct sockaddr *)&sa,sizeof(struct sockaddr_in)) < 0) {
		perror("sock bind ");
		close(sock);
		return(-1);
	}
	
	listen(sock, 100);
	
	return sock;
}


void *doForClientPool(void* arg){
	
	for(;;){

		int s;
		//sem_wait(&sem);

		if ((s = accept(sock,NULL,NULL)) < 0){		
			perror("get connection error ");
			return 0;
		}
		//sem_post(&sem);

		sem_wait(&sem);
		++current;
		fprintf(stdout,"Number of clients being served: %d\n",current);
		fflush(stdout);
		sem_post(&sem);

		m_sizes sizes;

		//fprintf(stdout,"client connected , socket : %d\n",s);

		if(readData(s,&sizes,sizeof(sizes)) == -1){
			perror("reading data failed ");
			return 0;
		}

//		printf("data received : row : %d , col : %d\n",sizes.row,sizes.col);

		long int tid = pthread_self();
		if(writeData(s,&tid,sizeof(tid)) == -1){
			perror("data writing failed ");
			return 0;
		}

		long int thTid;
		if(readData(s,&thTid,sizeof(thTid)) == -1){
			perror("data writing failed ");
			return 0;
		}
		//printf("Tid : %ld , Client Tid : %ld\n",tid,thTid);

		/*while(1){
			printf("tid : %ld working!\n",pthread_self());
			sleep(1);
		}*/


		doMatrixProcesses(sizes,s,thTid);


		sem_wait(&sem);
		--current;
		fprintf(stdout,"Number of clients being served: %d\n",current);
		fflush(stdout);
		sem_post(&sem);

	}

}


void *doForClient(void* arg){

	int s = *(int*)arg;

	sem_post(&sem);


	sem_wait(&sem);
	++current;
	fprintf(stdout,"Number of clients being served: %d\n",current);
	fflush(stdout);
	sem_post(&sem);



	m_sizes sizes;

	//fprintf(stdout,"client connected , socket : %d\n",s);

	if(readData(s,&sizes,sizeof(sizes)) == -1){
		perror("reading data failed ");
		return 0;
	}

	//printf("data received : row : %d , col : %d\n",sizes.row,sizes.col);

	long int tid = pthread_self();
	//printf("tid : %ld\n",tid);
	if(writeData(s,&tid,sizeof(tid)) == -1){
		perror("data writing failed ");
		return 0;
	}

	long int thTid;
	if(readData(s,&thTid,sizeof(thTid)) == -1){
		perror("data writing failed ");
		return 0;
	}
	//printf("thTid : %ld\n",thTid);

	doMatrixProcesses(sizes,s,thTid);


	sem_wait(&sem);
	--current;
	fprintf(stdout,"Number of clients being served: %d\n",current);
	fflush(stdout);
	sem_post(&sem);


}


void doMatrixProcesses(m_sizes sizes,int sock,long int tid){

	pthread_mutex_t shMutex = PTHREAD_MUTEX_INITIALIZER;

	pid_t p1,p2,p3;
	int shmid;

	int r = sizes.row , c = sizes.col;



	if((p1 = fork()) == -1){
		perror("Fork Failed! ");
		exit(0);
	}
	else if(p1 == 0){
		signal(SIGINT,childHandler);

		if((p2 = fork()) == -1){
			perror("Fork Failed! ");
			exit(0);
		}
	
		else if(p2 == 0){
			signal(SIGINT,childHandler);
			if((p3 = fork()) == -1){
				perror("Fork Failed! ");
				exit(0);
			}
			else if(p3 == 0){
				signal(SIGINT,childHandler);


				int i,j;
				double *shm,*ptr;
				double **A,**b,**x = generateMatris(c,1);
				double *temp = malloc(sizeof(double)*r*c);


				A = allocMatris(r,c);
				b = allocMatris(r,1);

				if((shmid = shmget(getppid(),sizeof(double),IPC_CREAT | 0666)) < 0){
					perror("shmget failed! ");
					exit(0);
				}
				
				if((shm = (double*)shmat(shmid,NULL,0)) == (double*)NULL){
					perror("shmat error ! ");
					exit(0);
				}

				FILE* log;
				char logName[BUFSIZE];
				snprintf(logName,BUFSIZE,"logs/server_%d_%ld.log",sock,tid);
				log = fopen(logName,"a+");

				//int end = 0; 
				//while(end++ != 3){
					i = 0;
					while(shm[r*c] != -1);

					ptr = shm;
					while(*ptr != -1){
						while(*ptr == 0);
						temp[i++] = *ptr;
						*ptr++;
					}
					*ptr = -2;
					
					//printf("----->received matris A2: \n");
					
					for(i = 0 ; i < r ; i++){
						for(j = 0 ; j < c ; j++)
							A[i][j] = temp[i*c + j];
					}

					//printMatris(A,r,c);

					///////CLIENT VERI GONDERME/////////
					if(writeData(sock,temp,sizeof(double)*r*c) == -1){
						perror("data writing failed ");
						return;
					}
					///////////////////////////////////////
					fprintf(log,"A : ");
					writeMatris(A,r,c,log);
					fflush(log);
					///////////////////////////////////////
					i = 0;
					while(shm[r*c] != -1);
			
					ptr = shm;
					while(*ptr != -1){
						while(*ptr == 0);
						temp[i++] = *ptr;
						*ptr++;
					}
					*ptr = -2;
					//printf("----->received matris b2: \n");
					
					
					for(i = 0 ; i < r ; i++){
						for(j = 0 ; j < 1 ; j++)
							b[i][j] = temp[i*c + j];
					}

					//printMatris(b,r,1);
					/////////////////////////////////////////
						
					///////CLIENT VERI GONDERME/////////
					if(writeData(sock,temp,sizeof(double)*r*c) == -1){
						perror("data writing failed ");
						return;
					}
					///////////////////////////////////////
					fprintf(log,"b : ");
					writeMatris(b,r,1,log);
					fflush(log);
					////////////////////////////////////////
					i = 0;
					while(shm[r*c] != -1 && shm[r*c] != -3);
			
					ptr = shm;
					while(*ptr != -1){
						while(*ptr == 0);
						temp[i++] = *ptr;
						*ptr++;
					}
					*ptr = -2;
					//printf("----->received matris x: \n");
					
					for(i = 0 ; i < c ; i++){
						
						for(j = 0 ; j < 1 ; j++){
							x[i][j] = temp[i + j];
						}
					}


				//}
				///////////////////////////////////// HATA -//////////////////
				//printf("----->received matris x: \n");

				//printMatris(x,c,1);


				///////CLIENT VERI GONDERME/////////
				if(writeData(sock,temp,sizeof(double)*r*c) == -1){
					perror("data writing failed ");
					return;
				}
				///////////////////////////////////////
				fprintf(log,"x : ");
				writeMatris(x,c,1,log);
				fflush(log);

				double **Ax = allocMatris(r,1);
				double **AxminusB = allocMatris(r,1);
				double **eT = allocMatris(1,r);
				double **eTe = allocMatris(1,1);

				multiple(A,r,c,x,c,1,Ax);

				//printf("Ax : \n");
				//printMatris(Ax,r,1);

				//printf("b:\n");
				//printMatris(b,r,1);

				Diff(Ax,b,r,1,AxminusB);

				//printf("Ax-b : \n");
				//printMatris(AxminusB,r,1);

				Transpose(AxminusB,eT,r,1);
				multiple(eT,1,r,AxminusB,r,1,eTe);

				//printf("error : %f\n",**eTe);
				**eTe = sqrt(**eTe);

				///////CLIENT VERI GONDERME/////////
				if(writeData(sock,*eTe,sizeof(double)) == -1){
					perror("data writing failed ");
					return;
				}
				///////////////////////////////////////
				///////////////////////////////////////
				free(temp);
				freeMatris(x,c);
				freeMatris(Ax,r);
				freeMatris(AxminusB,r);
				freeMatris(eT,1);
				freeMatris(eTe,1);

				freeMatris(A,r);
				freeMatris(b,r);

				fclose(log);

				shmctl(shmid, IPC_RMID, NULL);

				exit(0);

			}


			int i,j;
			double *shm,*ptr;
			double **A,**b;
			double *temp = malloc(sizeof(double)*r*c);

			A = allocMatris(r,c);
			b = allocMatris(r,1);

			if((shmid = shmget(getppid(),sizeof(double),IPC_CREAT | 0666)) < 0){
				perror("shmget failed! ");
				exit(0);
			}
			
			if((shm = (double*)shmat(shmid,NULL,0)) == (double*)NULL){
				perror("shmat error ! ");
				exit(0);
			}
			i = 0;
			while(shm[r*c] != -1);

			ptr = shm;
			while(*ptr != -1){
				while(*ptr == 0);
				temp[i++] = *ptr;
				*ptr++;
			}
			*ptr = -2;
			
			//printf("received matris A: \n");
			
			for(i = 0 ; i < r ; i++){
				for(j = 0 ; j < c ; j++)
					A[i][j] = temp[i*c + j];
			}

			//printMatris(A,r,c);

	///////////////////////////////////////////////////
			
			i = 0;
			while(shm[r*c] != -1);

			ptr = shm;
			while(*ptr != -1){
				while(*ptr == 0);
				temp[i++] = *ptr;
				*ptr++;
			}
			*ptr = -2;
			//printf("received matris b: \n");
			
			
			for(i = 0 ; i < r ; i++){
				for(j = 0 ; j < 1 ; j++)
					b[i][j] = temp[i*c + j];
			}

			//printMatris(b,r,1);

			shmctl(shmid, IPC_RMID, NULL);

	
			sArgs args1,args2,args3;
			
			pthread_t tid1,tid2,tid3;
			
	
			args1.method = SVD;
			args1.A = A;
			args1.b = b; 
			args1.key = getpid();
			args1.sizes = sizes;
			args1.shMutex = shMutex;
			pthread_create(&tid1,NULL,solve,(void*)&args1);
		

			sem_wait(&sem);
			args2.method = QR;
			args2.A = A;
			args2.b = b; 
			args2.key = getpid();
			args2.sizes = sizes;
			args2.shMutex = shMutex;
			pthread_create(&tid2,NULL,solve,(void*)&args2);
			
			
			sem_wait(&sem);
			args3.method = PSDINV;
			args3.A = A;
			args3.b = b; 
			args3.key = getpid();
			args3.sizes = sizes;
			args3.shMutex = shMutex;
			pthread_create(&tid3,NULL,solve,(void*)&args3);
			
			pthread_join(tid1,NULL);
			pthread_join(tid2,NULL);
			pthread_join(tid3,NULL);

			shmctl(shmid, IPC_RMID, NULL);



			free(temp);
			freeMatris(A,r);
			freeMatris(b,r);

			
	

	// ONLY P2 -------		
/*
			double **x;
			double* shm2;

			x = generateMatris(c,1);

			printf("generated result x : \n");
			printMatris(x,c,1);


			if((shmid = shmget(getpid(),sizeof(double),IPC_CREAT | 0666)) < 0){
				perror("shmget failed! ");
				exit(0);
			}

			if((shm = (double*)shmat(shmid,NULL,0)) == (double*)NULL){
				perror("shmat error ! ");
				exit(0);
			}


			for(i = 0 ; i < r ; i++)
				for(j = 0 ; j < c ; j++)
					shm[i*c + j] = A[i][j];

			shm[r*c] = -1;
			while(shm[r*c] != -2);

			//////////////////////////////////////////////

			for(i = 0 ; i < r ; i++)
				for(j = 0 ; j < 1 ; j++)
					shm[i*c + j] = b[i][j];

			shm[r*c] = -1;
			while(shm[r*c] != -2);


			for(i = 0 ; i < c ; i++)
				for(j = 0 ; j < 1 ; j++)
					shm[i + j] = x[i][j];

			shm[r*c] = -1;
			while(shm[i*c + j] != -2);

			freeMatris(x,1);
			shmctl(shmid, IPC_RMID, NULL);
*/
			exit(0);	 // END OF P2 /////////////////////////////////////////////
		}


		int i,j;
		double* shm;


		if((shmid = shmget(getpid(),sizeof(double),IPC_CREAT | 0666)) < 0){
			perror("shmget failed! ");
			exit(0);
		}

		if((shm = (double*)shmat(shmid,NULL,0)) == (double*)NULL){
			perror("shmat error ! ");
			exit(0);
		}
		double **A,**b;
		A = generateMatris(r,c);
		b = generateMatris(r,1);

		//printf("generated matris A: \n");
		//printMatris(A,r,c);

		//printf("generated matris b: \n");
		//printMatris(b,r,1);


		for(i = 0 ; i < r ; i++)
			for(j = 0 ; j < c ; j++)
				shm[i*c + j] = A[i][j];

		shm[r*c] = -1;
		while(shm[r*c] != -2);

//////////////////////////////////////////////

		for(i = 0 ; i < r ; i++)
			for(j = 0 ; j < 1 ; j++)
				shm[i*c + j] = b[i][j];

		shm[r*c] = -1;
		while(shm[r*c] != -2);
		
		freeMatris(A,r);
		freeMatris(b,r);

		shmctl(shmid, IPC_RMID, NULL);

		exit(0);
	}
	wait(NULL); // ------------------------------------
	pthread_mutex_destroy(&shMutex);
}


void *solve(void* arg){

	sArgs args = *(sArgs*)arg;

	sem_post(&sem);

	

	int r = args.sizes.row;
	int c = args.sizes.col;
	int method = args.method;
	int key = args.key;

	double** A = args.A;
	double** b = args.b;
	pthread_mutex_t shMutex = args.shMutex;
	
	


	double **x = generateMatris(c,1);

	int i,j;
	double* shm;
	int shmid;

	if(method == SVD || method == QR || method == PSDINV){

		pseudoInverse(A,b,r,c,x);

		//printf("--- result x (In Thread): \n");
		//printMatris(x,c,1);

	}



	sem_wait(&sem);
	//pthread_mutex_lock(&shMutex);


	if((shmid = shmget(key,sizeof(double),IPC_CREAT | 0666)) < 0){
		perror("shmget failed! ");
		exit(0);
	}

	if((shm = (double*)shmat(shmid,NULL,0)) == (double*)NULL){
		perror("shmat error ! ");
		exit(0);
	}


	for(i = 0 ; i < r ; i++)
		for(j = 0 ; j < c ; j++)
			shm[i*c + j] = A[i][j];

	shm[r*c] = -1;
	while(shm[r*c] != -2);

	//////////////////////////////////////////////

	for(i = 0 ; i < r ; i++)
		for(j = 0 ; j < 1 ; j++)
			shm[i*c + j] = b[i][j];

	shm[r*c] = -1;
	while(shm[r*c] != -2);


	for(i = 0 ; i < c ; i++)
		for(j = 0 ; j < 1 ; j++)
			shm[i + j] = x[i][j];

	shm[r*c] = -1;
	while(shm[i*c + j] != -2);
	

	freeMatris(x,1);
	
	sem_post(&sem);
}