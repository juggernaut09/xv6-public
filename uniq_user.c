#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"


char *arr[1024];
int length = 0;

// Memory Allocation
void init(){
    for(int i = 0; i < 1024; i++){
        arr[i] = malloc(100);
        }
}

int dataRead(char *fname){
    int file = 0, inp;
    int i, index=0, counter=0;
    char buffer[1024]; 
    char buffer1[1024];
    file = open(fname, O_RDONLY);
    if(file < 0){
        printf(1, "uniq: cannot open %s for reading: No such file or directory\n", fname);
        return -1;
    } 
    while((inp = read(file, buffer, 1024)) != 0){
        for (i = 0; i < inp; i++)
        {
            if(buffer[i] != '\n'){
                buffer1[index++] = buffer[i];      
            }else{
                strcpy(arr[counter++], buffer1);
                memset(buffer1, '\0', 1024);
                length++;
                index = 0;
            }
        }
    }
    return 0;
}

// Basic uniq functionality
void uniqbasic(){
    char buffer[100];
    for(int i = 0; i < length; i++){
        strcpy(buffer, arr[i]);  // Copying the strings to the buffer for the comparison
        if(strcmp(buffer, arr[i+1]) == 0){
            continue;  // If the consective lines are same, continue with the next strings
        }else{
            printf(1, "%s\n", buffer);
        }
    }
}


// keeping a count of the entered strings
void uniqcfunction(){
    char buffer[100];
    int count = 1; 
    for(int i = 0; i < length; i++){
        strcpy(buffer, arr[i]);
        if(strcmp(buffer, arr[i+1]) == 0){
            count++; // If both the lines are same, we increment the count  
            continue;
        }else{
            printf(1, "%d %s\n",count, buffer); 
            count = 1;
        }
    }
}


// printing only the duplicate lines
void uniqdfunction(){
    char buffer[100];
    int flag = 0;
    for(int i = 0; i < length; i++){
        strcpy(buffer, arr[i]);
        if(strcmp(buffer, arr[i+1]) == 0){
            flag = 1; // using flag to note that two lines are same
            continue;
        }else{
            if(flag == 1)
            printf(1, "%s\n", buffer);
            flag = 0;
        }
    }
}


// ignoring the case sensitivity 
void uniqifunction(){
    char buffer[100];
    char temp[100];
    char next[100];
    int i=0;
    for(i = 0; i < length; i++){
        strcpy(buffer, arr[i]);
        strcpy(temp, buffer);
        strcpy(next, arr[i + 1]);

        // converting all the characters in the string to the lower case 
        for(int j=0;buffer[j]!='\0';j++){
            if(buffer[j]>=65 && buffer[j]<=90){
                buffer[j]=buffer[j]+32;
            }
        }

        for(int j=0;next[j]!='\0';j++){
            if(next[j]>=65 && next[j]<=90){
                next[j]=next[j]+32;
            }
        }

        // comparing both the cases and printing only the unique string
        if(strcmp(buffer, next) == 0){
            continue;
        }else{
            printf(1, "%s\n", temp);
        }
    }
}

// redirecting the output of the cat command to input of the uniq 
void pipeFunction(){
    init();
    char buffer[1024],buffer1[1024];
        int inp,i, index=0;
        int counter=0;
        while((inp = read(0, buffer, sizeof(buffer))) > 0) {
            for (i = 0; i < inp; i++)
            {
                if(buffer[i] != '\n'){
                    
                    buffer1[index++] = buffer[i];
                        
                }else{
                    strcpy(arr[counter++], buffer1);
                    memset(buffer1, '\0', 1024);
                    length++;
                    index = 0;
                }
            }
        }
        uniqbasic();
}


// main function
int main(int argc, char* argv[]){
    printf(1,  "Uniq command is getting executed in user mode.\n");
    if(argc<=1){
        pipeFunction();
        }
    if(argc == 2){
        init();
        if(dataRead(argv[1]) < 0) exit();
        uniqbasic();
    }
    if(argc == 3){
        if(strcmp(argv[1], "-d") == 0){
             init();
        if(dataRead(argv[1]) < 0) exit();
        uniqdfunction();
        }
        if(strcmp(argv[1], "-i") == 0){
             init();
            if(dataRead(argv[1]) < 0) exit();
            uniqifunction();
        }
        if(strcmp(argv[1], "-c") == 0){
             init();
        if(dataRead(argv[1]) < 0) exit();
        uniqcfunction();
        }
    if(argc > 3){
        printf(1, "Too many arguments given");
    }
    }
   exit();
}