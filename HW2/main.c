#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


void findInMultipleFiles(char* mycwd , char* word,FILE* logFile,FILE* tmpFile); 
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
    char tmpPath[PATH_MAX] = "";	// gecici dosyanın pathi
    
   	int totalNumber = 0;			// toplam sozcuk sayisi
  	int tmp = 0;					// gecici degisken
  	
   	getcwd(logPath,PATH_MAX);
   	getcwd(tmpPath,PATH_MAX);
   	
   	strcat(logPath,"/log.log");
    strcat(tmpPath,"/tmp.txt");
   	
    chdir(filename);
	getcwd(mycwd,PATH_MAX);

	FILE* logFile = fopen(logPath,"w");
	FILE* tmpFile = fopen(tmpPath,"w+");


	findInMultipleFiles(mycwd,word,logFile,tmpFile);	// recursive fonku cagir
	while(wait(NULL) != -1);		// process olma ihtimaline karsı

	fseek(tmpFile,SEEK_SET,0);		// gecici dosyadaki sayiları topla : toplam sozcuk sayisi
    while(!feof(tmpFile))
    	if(fscanf(tmpFile,"%d",&tmp) == 1)
    		totalNumber += tmp;
    
    fprintf(logFile,"\n\n%d %s were found in total.\n",totalNumber,word);
    printf("\n\n%d %s were found in total.\n",totalNumber,word);
    
    fclose(logFile);		// log dosyasini kapat
    fclose(tmpFile);		// gecici dosyayi kapat
    remove(tmpPath);		// gecici dosyayi sil

    return 0;
}

void findInMultipleFiles(char* mycwd , char* word,FILE* logFile,FILE* tmpFile){
	struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    
    pid_t pid = 1;
    int result = -1;
    char path[PATH_MAX] = "";
    
    strcpy(path,mycwd);		// aktif pathi al
    
    printf("active directory: %s\n",mycwd);
    
    
	if((dirp = opendir(mycwd)) == NULL) { // dizini ac , acilmazsa hata bas ve cik
		printf("No file exists to be opened.\n");
		return;	
	}

    
	while((direntp = readdir(dirp)) != NULL){
        if(strcmp(direntp->d_name,".") != 0 && strcmp(direntp->d_name,"..") != 0 // ignore edilenler
        	&& direntp->d_name[strlen(direntp->d_name) - 1] != '~'){  
            
			pid = fork();	// yeni process
			if(pid < 0){
				perror("Fork failed\n	");
				closedir(dirp);
				return;
			}
			if(pid == 0){	// cocuksa
			    char tempPath[PATH_MAX];	// yeni(sonuna file ismi eklenecek) path
                strcpy(tempPath,path);
                strcat(tempPath,"/");;
                strcat(tempPath,direntp->d_name);
                
                if((direntp->d_type) == DT_DIR){//DIRECTORY ise
        	
                	printf("Directory : %s\n",direntp->d_name);
                	
                    result = chdir(tempPath);		// directorye gir
                    if(result == 0)
                        printf("entered : %s\n",getcwd(mycwd, PATH_MAX));
                    
				    findInMultipleFiles(mycwd,word,logFile,tmpFile); // recursive devam et
                    exit(0);
	            }         
                else if((direntp->d_type) == DT_REG){//FILE ise (txt,dat...)
				    printf("Reading file : '%s' ...\n",direntp->d_name);

		         	int tmpCount = findAll(tempPath,word,direntp->d_name,logFile); // dosyada kac tane bulundu
				    fprintf(tmpFile,"%d ",tmpCount);	// gecici dosyaya at
				    fflush(tmpFile);					// bufferi bosalt(cakisma olmasin)
		        	exit(0);
		        }
			
			}
			  	
	    }

	   else
	       printf("Other file : %s\n",direntp->d_name);

    }

	while(wait(NULL) != -1);		// herhangi bi islem varsa bekle ki dosyayı kapanmasın
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