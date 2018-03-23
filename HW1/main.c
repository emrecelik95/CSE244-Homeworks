#include <stdio.h>
#include <stdbool.h>

void findAll(char* filename , char* word);

int main(int argc , char** argv){
    // hatalı arguman kontrolu
    if(argc != 3){
        perror("Argumants must be three!");  
        return -1;  
    }

    char* filename = argv[2];
    char* word = argv[1];

    findAll(filename,word);

    return 0;
}

void findAll(char* filename , char* word){
    //  satır     sutun     aktif satır  aktif sutun  aktif ilk karakter
    int row = 1 , col = 0 , curRow = 0 , curCol = 0 , curPosFirst , i = 0 , total = 0;
    char c;   // dosya okurken kullanılacak char
    bool equals;    // kelime bulundumu
    FILE* file = fopen(filename,"r");
    if(file == NULL){               // dosya hata kontrolu
        perror("File couldn't be opened!\n");
        return;
    }
    
    if(word == NULL){
        perror("Word is null\n");
        return;
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
                printf("[%d, %d] konumunda ilk karakter bulundu.\n",curRow,curCol);
                ++total;
            }
            
            fseek(file,curPosFirst,SEEK_SET); // geri don (okumaya devam et)
        }
        
    } 
    fclose(file);
    printf("%d adet %s bulundu.\n",total,word);

}
