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
}params_t;


sem_t *sem;
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

long timedif;
struct timeval tpend;
struct timeval tpstart;

pthread_t tids[SIZE_MAX];
int i = 0;



static volatile sig_atomic_t doneflag = 0;



static void signalHandler(int signo) {
	doneflag = 1;

	if(signo == SIGINT){
		if(strcmp(exitCon,"") == 0){
			snprintf(exitCon,BUFSIZE,"due to signal %d",signo);
		}

		int tmp = 0,tmp2 = 0,tmp3 = 0,tmp4 = 0,dirCount = 0,fileCount = 0,lineCount = 0 ,i,j,max = 0;
	    int cascade[BUFSIZE];


	    if(access(tmpPath,F_OK) == -1)
	    	exit(0);

	    fseek(tmpFile,SEEK_SET,0);      // gecici dosyadaki sayiları topla
	    while(!feof(tmpFile)){
	    	if(fscanf(tmpFile,"%d %d %d %d\n",&tmp,&tmp2,&tmp3,&tmp4) == 4){
	    		totalNumber += tmp;
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
	    printf("Exit condition: %s\n",exitCon);

	    int k;
	    for(k = 0 ; k < i ; k++)
	        pthread_join(tids[k],NULL);


	    fclose(logFile);		// log dosyasini kapat
	    fclose(tmpFile);		// gecici dosyayi kapat
	    remove(tmpPath);		// gecici dosyayi sil

	    fclose(tmpCasc);
	    remove(cscPath);
	    
	    sem_close(sem);
	    sem_unlink(sName);
	}
	else{	
		raise(SIGINT);
	}

    exit(0);
}

int main(int argc, char *argv[]) {
    
	if(argc != 3){
        fprintf(stderr,"Usage : ./executable 'string' 'directory name'\n");
		return 1;
    }
    
    char* filename = argv[2];
    word = argv[1];
    char mycwd[PATH_MAX] = "";		//current directory
    int maindPid;  

    if (gettimeofday(&tpstart, NULL)) {
        fprintf(stderr, "Failed to get start time\n");
        return 1;
    }


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

    createSem(sName,&sem,1);/////////////////////////////////////////////////////////////////


    signal(SIGINT,signalHandler);
    signal(SIGTERM,signalHandler);
    signal(SIGUSR1,signalHandler);
    signal(SIGUSR2,signalHandler);


	findInMultipleFiles(mycwd,word,logFile);	// recursive fonku cagir


	strcpy(exitCon,"normal");
    raise(SIGINT);


    return 0;
}

void findInMultipleFiles(char* mycwd , char* word,FILE* logFile){
	struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    
    pid_t pid = 1;
    int result = -1;
    char path[PATH_MAX] = "";
    

    strcpy(path,mycwd);		// aktif pathi al
    
    //printf("active directory: %s\n",mycwd);

	if((dirp = opendir(mycwd)) == NULL) { // dizini ac , acilmazsa hata bas ve cik
		printf("No file exists to be opened.\n");
		snprintf(exitCon,BUFSIZE,"due to error %d",errno);
        raise(SIGINT);	
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
                    fprintf(tmpFile,"%d %d %d %d\n",0,0,1,0);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
                    fflush(tmpFile);                // bufferi bosalt(cakisma olmasin)

                    pthread_attr_t attr;
                    pthread_attr_init(&attr);
                    
					while (sem_wait(sem) == -1)
				        if(errno != EINTR) {
				        	fprintf(stderr, "Thread failed to lock semaphore\n");
				        	snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		        			raise(SIGINT);
			    		}

                    params_t params;	

                    strcpy(params.tempPath,tempPath);
                    strcpy(params.word,word);
                    strcpy(params.d_name,direntp->d_name);
                    params.logFile = logFile;

                    
                    pthread_create(&tids[i++],&attr,threadHandler,(void*)&params);

		        }
                else if((direntp->d_type) == DT_DIR){//DIRECTORY ise
                    fprintf(tmpFile,"%d %d %d %d\n",0,1,0,0);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
                    fflush(tmpFile);                // bufferi bosalt(cakisma olmasin)

                    pid = fork();   // yeni process
                    if(pid < 0){
                        perror("Fork failed\n   ");
                        closedir(dirp);
                        snprintf(exitCon,BUFSIZE,"due to error %d",errno);
	        			raise(SIGINT);
                    }
                    if(pid == 0){
                    	signal(SIGINT,SIG_DFL);
                    	signal(SIGTERM,SIG_DFL);
					    signal(SIGUSR1,SIG_DFL);
					    signal(SIGUSR2,SIG_DFL);

                    	//printf("Directory : %s\n",direntp->d_name);
                
                        result = chdir(tempPath);		// directorye gir
                        //if(result == 0)
                            //printf("entered : %s\n",getcwd(mycwd, PATH_MAX));
                        getcwd(mycwd, PATH_MAX);
    				    findInMultipleFiles(mycwd,word,logFile); // recursive devam et
    				    
    				    fclose(tmpFile);
    				    fclose(tmpCasc);
    				    fclose(logFile);
    				    sem_close(sem);
                        exit(0);
                    }
	            }         
                
						  	
	    }

	    /*else
	        printf("Other file : %s\n",direntp->d_name);*/

    }

	while(wait(NULL) != -1);		// herhangi bi islem varsa bekle ki dosyayı kapanmasın

	if(i > 0){
	    fprintf(tmpCasc,"%d ",i);
	    fflush(tmpCasc);
	    int k;
	    for(k = 0 ; k < i ; k++)
	        pthread_join(tids[k],NULL);
    
	}
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

    fprintf(tmpFile,"%d %d %d %d\n",count,0,0,0);   // gecici dosyaya at , (kelime sayısı , dir sayisi , file sayisi , satir sayisi
    fflush(tmpFile);                // bufferi bosalt(cakisma olmasin)

    if (sem_post(sem) == -1){
        fprintf(stderr, "Thread failed to unlock semaphore\n");
        snprintf(exitCon,BUFSIZE,"due to error %d",errno);
		raise(SIGINT);
    }
    
    return NULL;
}

int createSem(char *name, sem_t **sem, int val) {
    while (((*sem = sem_open(name, FLAGS, PERMS, val)) == SEM_FAILED) &&
                                                             (errno == EINTR)) ;
    if (*sem != SEM_FAILED)
        return 0;
    if (errno != EEXIST)
        return -1;
    while (((*sem = sem_open(name, 0)) == SEM_FAILED) && (errno == EINTR)) ;
        if (*sem != SEM_FAILED)
            return 0;
    return -1;
}