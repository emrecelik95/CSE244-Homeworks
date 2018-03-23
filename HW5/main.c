#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>


#define MILLION 1000000L

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define SIZE_MAX 100000

#define BUFSIZE 100
#define FIFO_PERM (S_IRUSR | S_IWUSR)

#define PERMS (mode_t)(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define FLAGS (O_CREAT | O_EXCL)


void findInMultipleFiles(char* mycwd , char* word,FILE* logFile); 
int findAll(char* path , char* word,char* thisFile,FILE* logFile);


void* threadHandler(void* arg);
int createSem(char *name, sem_t **sem, int val);

typedef struct params_t{
    char tempPath[PATH_MAX];
    char word[BUFSIZE];
    char d_name[PATH_MAX];
    FILE* logFile;
    int* count;
}params_t;

sem_t sem;
char* sName = "semf";
char logPath[PATH_MAX] = "";	// log un pathi
char tmpPath[PATH_MAX] = "";	// gecici dosyanın pathi
char cscPath[PATH_MAX] = "";
char* word;
FILE* tmpFile;
FILE* tmpCasc;
FILE* logFile;
char exitCon[BUFSIZE] = "";

int totalNumber = 0;			// toplam sozcuk sayisi
int msqidUp;

int shmid;
int msqid;

long timedif;
struct timeval tpend;
struct timeval tpstart;

pthread_t tids[SIZE_MAX];
int i = 0;



static volatile sig_atomic_t doneflag = 0;

static void signalHandlerCH(){

    msgctl(msqidUp, IPC_RMID, NULL);
    msgctl(msqid, IPC_RMID, NULL);
    shmctl(shmid, IPC_RMID, NULL);

    fclose(tmpFile);
    fclose(tmpCasc);
    fclose(logFile);
    sem_close(&sem);
    shmctl(shmid,IPC_RMID,NULL);
    exit(0);

}

static void signalHandler(int signo) {
	doneflag = 1;

	if(signo == SIGINT){
		snprintf(exitCon,BUFSIZE,"due to signal %d",signo);
	}
    else if(signo == SIGTERM){
        if(strcmp(exitCon,"") == 0){
            snprintf(exitCon,BUFSIZE,"due to signal %d",signo);
        }

        int tmp = 0,tmp2 = 0,tmp3 = 0,tmp4 = 0,dirCount = 0,fileCount = 0,lineCount = 0 ,i,j,max = 0;
        int cascade[BUFSIZE];
        int msgCount = 0;


        if(access(tmpPath,F_OK) == -1)
            exit(0);

         while(msgrcv(msqidUp, &msgCount, sizeof(int), 0, PERMS | O_NONBLOCK) != -1){
            totalNumber += msgCount;
         }

         while(msgrcv(msqid, &msgCount, sizeof(int), 0, PERMS | O_NONBLOCK) != -1){
            totalNumber += msgCount;
         }

        msgctl(msqidUp, IPC_RMID, NULL);
        msgctl(msqid, IPC_RMID, NULL);
        shmctl(shmid, IPC_RMID, NULL);

        dirCount = 0;
        fileCount = 0;
        lineCount = 0;
        fseek(tmpFile,SEEK_SET,0);      // gecici dosyadaki sayiları topla
        while(!feof(tmpFile)){
            if(fscanf(tmpFile,"%d %d %d %d\n",&tmp,&tmp2,&tmp3,&tmp4) == 4){
                //totalNumber += tmp;   (onceki hali)
                dirCount += tmp2;
                fileCount += tmp3;
                lineCount += tmp4;
            }
        }

        fseek(tmpCasc,SEEK_SET,0);      // gecici dosyadaki sayiları topla
        i = 0;
        while(!feof(tmpCasc)){
            if(fscanf(tmpCasc,"%d ",&cascade[i++]));
        }

        if (gettimeofday(&tpend, NULL)) {
            fprintf(stderr, "Failed to get end time\n");
            snprintf(exitCon,BUFSIZE,"due to error %d",errno);
            raise(SIGINT);
        }

        timedif = MILLION*(tpend.tv_sec - tpstart.tv_sec) +
        tpend.tv_usec - tpstart.tv_usec;
        
        fprintf(logFile,"\n\n%d %s were found in total.\n",totalNumber,word);
        printf("Total Number of '%s' found :  %d\n",word,totalNumber);
        printf("Number of directories searched :  %d\n",dirCount);
        printf("Number of files searched :  %d\n",fileCount);
        printf("Number of lines searched :  %d\n",lineCount);
        printf("Number of cascade threads created :  ");
        for(j = 0 ; j < i ; ++j){
            printf("%d ",cascade[j]);
            if(cascade[j] > max)
                max = cascade[j];
        }
        printf("\n");
        printf("Number of search threads created: :  %d\n",fileCount);
        printf("Max %d of threads running concurrently\n",max);
        printf("Total run time : %ld milliseconds.\n", timedif / 1000);
        printf("Total number of shared memory created : %d\n",dirCount);
        printf("Sizes of each shared memory : ");
        i = 0;
        while(i < dirCount){
            printf("%ld ",sizeof(int));
            ++i;
        }
        printf("\n");
        printf("Exit condition: %s\n",exitCon);

        int k;
        for(k = 0 ; k < i ; k++)
            pthread_join(tids[k],NULL);


        fclose(logFile);        // log dosyasini kapat
        fclose(tmpFile);        // gecici dosyayi kapat
        remove(tmpPath);        // gecici dosyayi sil

        fclose(tmpCasc);
        remove(cscPath);
        
        sem_close(&sem);
        sem_unlink(sName);

        exit(0);
    }
	else{	
        snprintf(exitCon,BUFSIZE,"due to signal %d",signo);
		raise(SIGTERM);
	}

    
}

int main(int argc, char *argv[]) {
    
	if(argc != 3){
        fprintf(stderr,"Usage : ./executable 'string' 'directory name'\n");
		return 1;
    }
    
    char* filename = argv[2];
    word = argv[1];
    char mycwd[PATH_MAX] = "";		//current directory
    char startdir[PATH_MAX] = "";

    if (gettimeofday(&tpstart, NULL)) {
        fprintf(stderr, "Failed to get start time\n");
        return 1;
    }

    getcwd(startdir,PATH_MAX);

   	getcwd(logPath,PATH_MAX);
   	getcwd(tmpPath,PATH_MAX);
   	getcwd(cscPath,PATH_MAX);

   	strcat(logPath,"/log.txt");
    strcat(tmpPath,"/tmp.txt");
   	strcat(cscPath,"/casc.txt");

    logFile = fopen(logPath,"w");

	tmpCasc = fopen(cscPath,"w+");


    chdir(filename);
	getcwd(mycwd,PATH_MAX);

	tmpFile = fopen(tmpPath,"w+");

    sem_init(&sem,0,1);

    signal(SIGINT,signalHandler);
    signal(SIGTERM,signalHandler);
    signal(SIGUSR1,signalHandler);
    signal(SIGUSR2,signalHandler);


	findInMultipleFiles(mycwd,word,logFile);	// recursive fonku cagir

    if(!doneflag)
	   strcpy(exitCon,"normal");
    
    raise(SIGTERM);


    return 0;
}

void findInMultipleFiles(char* mycwd , char* word,FILE* logFile){
	struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    
    pid_t pid = 1;

    int * shmCount = 0;

    char path[PATH_MAX] = "";

    strcpy(path,mycwd);		// aktif pathi al
    
    //printf("active directory: %s\n",mycwd);

	if((dirp = opendir(mycwd)) == NULL) { // dizini ac , acilmazsa hata bas ve cik
		printf("No file exists to be opened.\n");
		snprintf(exitCon,BUFSIZE,"due to error %d",errno);
        raise(SIGINT);	
	}


    shmid = shmget(IPC_PRIVATE,sizeof(int), IPC_CREAT | 0666);
    if(shmid < 0)
    {
        perror("shmget failed");
        exit(0);
    }

    shmCount = shmat(shmid,NULL,0);

    if(shmCount == (int*)-1){
        perror("shmat");
        exit(0);
    }

    if ((msqid = msgget(getpid(), PERMS | IPC_CREAT)) == -1){
        perror("Failed to create new message queue");
        exit(0);
    }
	while((direntp = readdir(dirp)) != NULL && !doneflag){
        if(strcmp(direntp->d_name,".") != 0 && strcmp(direntp->d_name,"..") != 0 // ignore edilenler
        	&& direntp->d_name[strlen(direntp->d_name) - 1] != '~'){  
        		
        		if(doneflag)
        			exit(0);


			    char tempPath[PATH_MAX];	// yeni(sonuna file ismi eklenecek) path
                strcpy(tempPath,path);
                strcat(tempPath,"/");
                strcat(tempPath,direntp->d_name);
                
                if((direntp->d_type) == DT_REG){//FILE ise (txt,dat...)

				    //printf("Reading file : '%s' ...\n",direntp->d_name);
                    
                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    
					while (sem_wait(&sem) == -1)
				        if(errno != EINTR) {
				        	fprintf(stderr, "Thread failed to lock semaphore\n");
				        	snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		        			raise(SIGINT);
			    		}

                    params_t params;	

                    strcpy(params.tempPath,tempPath);
                    strcpy(params.word,word);
                    strcpy(params.d_name,direntp->d_name);
                    params.count = shmCount;
                    params.logFile = logFile;

                    
                    pthread_create(&tids[i++],&attr,threadHandler,(void*)&params);

		        }
                else if((direntp->d_type) == DT_DIR){//DIRECTORY ise

                    pid = fork();   // yeni process
                    if(pid < 0){
                        perror("Fork failed\n   ");
                        closedir(dirp);
                        snprintf(exitCon,BUFSIZE,"due to error %d",errno);
	        			raise(SIGINT);
                    }
                    if(pid == 0){
                    	signal(SIGINT,signalHandlerCH);
                    	signal(SIGTERM,signalHandlerCH);
					    signal(SIGUSR1,signalHandlerCH);
					    signal(SIGUSR2,signalHandlerCH);

                    	//printf("Directory : %s\n",direntp->d_name);
                
                        chdir(tempPath);		// directorye gir

                        getcwd(mycwd, PATH_MAX);
    				    findInMultipleFiles(mycwd,word,logFile); // recursive devam et      

    				    fclose(tmpFile);
    				    fclose(tmpCasc);
    				    fclose(logFile);
    				    sem_close(&sem);
                        shmctl(shmid,IPC_RMID,NULL);
                        exit(0);
                    }
	            }         
                
						  	
	    }

	    /*else
	        printf("Other file : %s\n",direntp->d_name);*/

    }

	while(wait(NULL) != -1);		
 
	if(i > 0){
	    fprintf(tmpCasc,"%d ",i);
	    fflush(tmpCasc);
	    int k;
	    for(k = 0 ; k < i ; k++){
	        pthread_join(tids[k],NULL);
            fprintf(tmpFile,"%d %d %d %d\n",0,0,1,0);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
            fflush(tmpFile);                // bufferi bosalt(cakisma olmasin)

        }
    
	}
    shmctl(shmid,IPC_RMID,NULL);
/*
    printf("shared  , count : %d\n",*shmCount);
    printf("path : %s \npid : %d , ppid : %d\n",path,getpid(),getppid());
*/
    int temp = 0,tempTotal = 0;
    while(msgrcv(msqid, &temp, sizeof(int), 0, PERMS | O_NONBLOCK) != -1){
        //printf("current received : %d \n",temp);
        tempTotal += temp;
    }

    if ((msqidUp = msgget(getppid(), PERMS | IPC_CREAT)) == -1){
        perror("Failed to create new message queue");
        exit(0);
    }

    tempTotal += *shmCount;
    int tempSend = tempTotal;
    if(msgsnd(msqidUp, &tempSend, sizeof(int), 0) == -1){
        perror("msgsnd failed");
        exit(0);
    }
    
    if(msgctl(msqid, IPC_RMID, NULL) == -1){
        perror("msgctl failed");
        exit(0);
    }
    
    fprintf(tmpFile,"%d %d %d %d\n",0,1,0,0);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
    fflush(tmpFile);                // (cakisma olmasin)

	while((closedir(dirp) == -1) && errno == EINTR);	
	
}


int findAll(char* path , char* word,char* thisFile,FILE* logFile){
    //  satır     sutun     aktif satır  aktif sutun  aktif ilk karakter
    int row = 1 , col = 0 , curRow = 0 , curCol = 0 , curPosFirst , i = 0 , total = 0 , lineNum = 0;
    char c;   // dosya okurken kullanılacak char
    bool equals = false;    // kelime bulundumu
    FILE* file = fopen(path,"r");
    
    if(logFile == NULL){			// log dosyası pointeri null ise
    	perror("Log file couldn't be opened!\n");
    	snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		raise(SIGINT);
    }
    
    if(file == NULL){               // dosya hata kontrolu
        perror("File couldn't be opened!\n");
        snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		raise(SIGINT);
    }
    
    if(word == NULL){				// kelime null ise
        perror("Word is null\n");
        snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		raise(SIGINT);
    }
    
    while((c = fgetc(file)) != EOF){        // dosyanın sonun kadar oku
        ++col;                                          // sutun artırma
        while((c == ' ' || c == '\t') && !feof(file)){  // ignore lar
            (c = fgetc(file));
            ++col;                                      // sutun artırma
        }
        while(c == '\n'){                               // satır artırma
            c = fgetc(file); 
            ++row;
            col = 1;                                    // sutun 1 oldu
            lineNum++;
        }
        if(c == word[0]){        // ilki eslesti 
            equals = true;
            curRow = row;        // satır ve sutun gecici degiskenlere kopyala
            curCol = col;    
            curPosFirst = ftell(file);  // bulundugu konumu tut
            
            // bosluk endline tab haricindekileri kelimeyle karsılastır
            for(i = 1 ; word[i] != '\0' && (c = fgetc(file)) != EOF ; ++i){

                while((c == ' ' || c == '\t' || c == '\n') && !feof(file))
                    c = fgetc(file);
                                                                          
                if(c != word[i])
                    equals = false;
            }
            // dosya sonuna cabuk geldiyse tam karsılastırmamıs olabilir
            if(word[i] != '\0')  
                equals = false;
            if(equals){
                fprintf(logFile,"PID:%ld - TID:%ld  %s : [%d,%d] %s first character is found.\n",(long int)getpid(),pthread_self(),thisFile,curRow,curCol,word);     // dosyaya yaz
                fflush(logFile);		// bufferi bosalt ki diger processlerle cakisma olmasin
                ++total;				// count u arttır
            }
            
            fseek(file,curPosFirst,SEEK_SET); // geri don (okumaya devam et)
        }
        
    } 
    fprintf(tmpFile,"%d %d %d %d\n",0,0,0,lineNum);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
    fflush(tmpFile);                // bufferi bosalt(cakisma olmasin)

    fclose(file); 	// dosyayı kapat 
	
    return total;
}



void* threadHandler(void* arg){
	

    params_t params = *(params_t *)arg;
    
    int count = findAll(params.tempPath,params.word,params.d_name,params.logFile); // dosyada kac tane bulundugu
    *(params.count) += count;

    fprintf(tmpFile,"%d %d %d %d\n",count,0,0,0);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
    fflush(tmpFile);                // bufferi bosalt(cakisma olmasin)

    if (sem_post(&sem) == -1){
        fprintf(stderr, "Thread failed to unlock semaphore\n");
        snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		raise(SIGINT);
    }
    
    return NULL;
}
