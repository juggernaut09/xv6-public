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
    // This function intitializes arr which is an array of strings 
    //["I understand the Operating system.", "I understand the Operating system.", "I understand the Operating system.", 
    //"I love to work on OS.", "I love to work on OS.", "Thanks xv6"].
    int file = 0, number_of_bytes_read;
    int i, index=0, counter=0;
    char buffer[1024]; 
    char buffer1[1024];
    file = open(fname, O_RDONLY);
    if(file < 0){
        printf(1, "uniq: cannot open %s for reading: No such file or directory\n", fname);
        return -1;
    }  
    // no of bytes to read = 1024
    // buffer will have 1024 bytes read in a single stride.
    while((number_of_bytes_read = read(file, buffer, 1024)) != 0){
        for (i = 0; i < number_of_bytes_read; i++) // Looping through the characters read in single stride i.e. 1024 characters
        {
            if(buffer[i] != '\n'){ // Check if the reading character is new line sequence or not
                buffer1[index] = buffer[i]; // if it is a valid character then store in the buffer1 char array.
                index = index + 1; // increment the index.     
            }else{ // If it reaches the new line i.e a complete string.
                strcpy(arr[counter], buffer1); //Store that string in arr.
                counter = counter + 1;
                memset(buffer1, '\0', 1024); // Make the buffer1 empty. remove the string from buffer1 which was just now moved to arr.  
                length++;
                index = 0;
            }
        }
    }
    return 0;
}



// redirecting the output of the cat command to input of the uniq 
void pipeFunction(){
    init();
    char read_buffer[1024];
    char string[1024];
        int number_of_bytes_read,i, index=0;
        int counter=0;
        // Reading file contents, we could have used the dataRead function 
        // But we are passing fd instead of filename.
        while((number_of_bytes_read = read(0, read_buffer, sizeof(read_buffer))) > 0) {
            // char takes 1 byte exactly in C. 
            //So we are reading 1024 characters in single stride.
            for (i = 0; i < number_of_bytes_read; i++)
            {
                if(read_buffer[i] != '\n'){
                    string[index] = read_buffer[i]; // Storing each character in the string
                    index = index + 1;                        
                }else{ // If it is newline.
                    strcpy(arr[counter], string); //Store the string in arr of strings.
                    counter = counter + 1;
                    memset(string, '\0', 1024); //Empty the String buffer.
                    length = length + 1;
                    index = 0;
                }
            }
        }
}

int 
main(int argc, char* argv[]) {
    // printf(1, "argc : %d", )
    // for(int i=0; i< argc; i++){
    //     printf(1, "%s\n", argv[i]);
    // }
    char *flag;
    char *arr_of_str[30];
    if(argc <= 1){
        flag = "basic";
        pipeFunction();
    }
    if(argc == 2){
        flag = "basic";
        init();
        if(dataRead(argv[1]) < 0) exit();
    }
    if(argc == 3){   
        flag = argv[1];
        init();
        if(dataRead(argv[2]) < 0) exit();
    }
    // if(argc > 3){
    //     char *nargv[3];
    //     for(int i=0; i< 3; i++){
    //         nargv[i] = argv[i];
    //     }
    //     flag = argv[1];
    //     init();
    //     if(dataRead(nargv[2]) < 0) exit();
    // }
    for(int i = 0; i< 30; i++){
        arr_of_str[i] = arr[i];
    }
    uniq(flag, arr_of_str, length);
    exit();
 }