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

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BUFSIZE 15
#define FIFO_PERM (S_IRUSR | S_IWUSR)

void findInMultipleFiles(char* mycwd , char* word,FILE* logFile,char* fifoPath); 
int findAll(char* path , char* word,char* thisFile,FILE* logFile);

int main(int argc, char *argv[]) {
    
	if(argc != 3){
        fprintf(stderr,"Usage : ./executable 'string' 'directory name'\n");
		return 1;
    }
    
    char* filename = argv[2];
    char* word = argv[1];
    char mycwd[PATH_MAX] = "";		//current directory
    char logPath[PATH_MAX] = "";	// log un pathi
    char fifoPath[PATH_MAX] = "";	// fifo nun pathi
    char* fifoname = "myfifo";		// fifoismi

   	int totalNumber = 0;			// toplam sozcuk sayisi
  	int tmp = 0;					// gecici degisken
  	int pid;					

   	getcwd(logPath,PATH_MAX);
   	getcwd(fifoPath,PATH_MAX);

   	strcat(fifoPath,"/");
   	strcat(fifoPath,fifoname);

   	strcat(logPath,"/log.log");
   	
	if (mkfifo(fifoPath, FIFO_PERM) == -1){ // FIFO
		if (errno != EEXIST) { 				// OLUSMAZSA
			fprintf(stderr, "[%ld]:failed to create named pipe %s: %s\n",
				(long)getpid(), argv[1], strerror(errno));
			return 1;
		}
		perror("Failed to create myfifo");
	}

    chdir(filename);
	getcwd(mycwd,PATH_MAX);

	FILE* logFile = fopen(logPath,"w");
	

	if((pid = fork()) == -1){ // FORK (FIFOYU ES ZAMANLI OKUYABILMEK ICIN PROCESS)
		perror("Failed to fork\n");
		return 1;
	}
	else if (pid > 0){ // PARENT FIFOYU PARALEL PROCESS OLARAK OKUYUP TOTAL NUMBERI HESAPLAR
		int fds;
		int tmpCount = 0;
		char buf[BUFSIZE];

		while (((fds = open(fifoPath, O_RDONLY)) == -1) && (errno == EINTR)) ; // FIFOYU OKUMA MODUNDA AC
		if (fds == -1) {
			fprintf(stderr, "[%ld]:failed to open named pipe %s for write: %s\n",
			(long)getpid(), fifoPath, strerror(errno));
			return 1;
		}

		while(read(fds,&tmpCount,sizeof(tmpCount)) > 0){ // OKUNUYORSA OKU VE TOTAL COUNTU ARTTIR
			totalNumber += tmpCount;
		}
		while(wait(NULL) != -1);
		close(fds); // FIFO YU KAPAT

	    fprintf(logFile,"\n\n%d %s were found in total.\n",totalNumber,word); // DOSYAYA YAZ

		fclose(logFile);		// log dosyasini kapat
    	unlink(fifoPath);

		return 1;

	}
	else{

		findInMultipleFiles(mycwd,word,logFile,fifoPath);	// recursive fonku cagir

	    return 1;
	}

    return 0;
}

void findInMultipleFiles(char* mycwd , char* word,FILE* logFile,char* fifoPath){
	struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    
    pid_t pid = 1;
    int result = -1;
    char path[PATH_MAX] = "";
  	int count;
    int fd[2];
	int fds;

    strcpy(path,mycwd);		// aktif pathi al
        
    
	if((dirp = opendir(mycwd)) == NULL) { // dizini ac , acilmazsa hata bas ve cik
		printf("No file exists to be opened.\n");
		return;	
	}

	while (((fds = open(fifoPath, O_RDWR)) == -1) && (errno == EINTR)); // FIFOYU YAZMA,OKUMA MODUNDA AC
	if (fds == -1) {
		fprintf(stderr, "(REC) , [%ld]:failed to open named pipe %s for write: %s\n",
						(long)getpid(), fifoPath, strerror(errno));
		return;
	}

	while((direntp = readdir(dirp)) != NULL){
        if(strcmp(direntp->d_name,".") != 0 && strcmp(direntp->d_name,"..") != 0 // ignore edilenler
        	&& direntp->d_name[strlen(direntp->d_name) - 1] != '~'){  
            
        	if((direntp->d_type) == DT_REG){ // HER DOSYA ICIN PIPE  
        			if (pipe(fd) == -1) { 
					perror("Failed to create the pipe");
					return;
				}	
			}
			char tempPath[PATH_MAX];	// yeni(sonuna file ismi eklenecek) path
            strcpy(tempPath,path);
            strcat(tempPath,"/");
            strcat(tempPath,direntp->d_name);

			pid = fork();	// yeni process
			if(pid < 0){
				perror("Fork failed\n	");
				closedir(dirp);
				return;
			}
			if(pid == 0){	// CHILD		

				if((direntp->d_type) == DT_REG){//FILE ise (txt,dat...)

		         	count = findAll(tempPath,word,direntp->d_name,logFile); // dosyada kac tane bulundugu

		         	close(fd[0]);
		         	write(fd[1],&count,sizeof(count)); // COUNTU PIPE'A YAZ
		         	close(fd[1]);
		        }
		        if((direntp->d_type) == DT_DIR){//DIRECTORY ise
        			
                	
                    result = chdir(tempPath);		// directorye gir
                    getcwd(mycwd, PATH_MAX);				

                    strcpy(fifoPath,mycwd);
                    strcat(fifoPath,"/");
                    char tmp[BUFSIZE];
                    snprintf(tmp,BUFSIZE,"%d",getpid());//YENI ISIMDE FIFO (O DIRECTORY'NIN PID'SINE OZEL)

                    strcat(fifoPath,tmp); // YENI FIFONUN PATH'I

                    if (mkfifo(fifoPath,FIFO_PERM) == -1){ // YENI FIFO OLUSTUR
						if (errno != EEXIST) { 				// OLUSMAZSA
							fprintf(stderr, "[%ld]:failed to create named pipe %s\n",
								(long)getpid(),fifoPath);
						}
						perror("Failed to create myfifo");
					}
				    findInMultipleFiles(mycwd,word,logFile,fifoPath); // recursive devam et
				    int fds2 = open(fifoPath, O_RDONLY | O_NONBLOCK); // ALTTAKI FIFONUN USTE BILGI AKTARMASI ICIN 
				    int tmpTotal = 0;
				    while(read(fds2,&count,sizeof(count)) > 0){ // OKUMA
				    	tmpTotal += count;
				    }
				    close(fds2); 
				    write(fds,&tmpTotal,sizeof(tmpTotal)); // USTTEKINE BILGI AKTAR
				    unlink(fifoPath);	// FIFOYU KALDIR
	            } 
		        exit(0);
			}
			else{

			    if((direntp->d_type) == DT_REG){ // dosyalar icin pipe'i al ve fifoya yaz
					close(fd[1]);
			    	read(fd[0], &count, sizeof(count)); // COUNTU PIPE DAN OKU
			    	close(fd[0]);
				
					write(fds,&count,sizeof(count)); // FIFOYA YAZ
				}
			}
	    }

    }

	while(wait(NULL) != -1);		// herhangi bi islem varsa bekle ki dosyayı kapamasın
	while((closedir(dirp) == -1) && errno == EINTR);	
	
}


int findAll(char* path , char* word,char* thisFile,FILE* logFile){
    //  satır     sutun     aktif satır  aktif sutun  aktif ilk karakter
    int row = 1 , col = 0 , curRow = 0 , curCol = 0 , curPosFirst , i = 0 , total = 0;
    char c;   // dosya okurken kullanılacak char
    bool equals = false;    // kelime bulundumu
    FILE* file = fopen(path,"r");
    
    if(logFile == NULL){			// log dosyası pointeri null ise
    	perror("Log file couldn't be opened!\n");
    	return 0;
    }
    
    if(file == NULL){               // dosya hata kontrolu
        perror("File couldn't be opened!\n");
        return 0;
    }
    
    if(word == NULL){				// kelime null ise
        perror("Word is null\n");
        return 0;
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
                fprintf(logFile,"%s : [%d,%d] %s first character is found.\n",thisFile,curRow,curCol,word);	 // dosyaya yaz
                fflush(logFile);		// bufferi bosalt ki diger processlerle cakisma olmasin
                ++total;				// count u arttır
            }
            
            fseek(file,curPosFirst,SEEK_SET); // geri don (okumaya devam et)
        }
        
    } 
    fclose(file); 	// dosyayı kapat 
	
    return total;
}
