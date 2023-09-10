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

void dataRead(char *fname){
    // This function intitializes arr which is an array of strings 
    //["I understand the Operating system.", "I understand the Operating system.", "I understand the Operating system.", 
    //"I love to work on OS.", "I love to work on OS.", "Thanks xv6"].
    int file = 0, number_of_bytes_read;
    int i, index=0, counter=0;
    char buffer[1024]; 
    char buffer1[1024];
    file = open(fname, O_RDONLY);
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
}

// Basic uniq functionality
// void uniqbasic(){
//     char buffer[100];
//     for(int i = 0; i < length; i++){
//         strcpy(buffer, arr[i]);  // Copying the strings to the buffer for the comparison
//         if(strcmp(buffer, arr[i+1]) == 0){
//             continue;  // If the consective lines are same, continue with the next strings
//         }else{
//             printf(1, "%s\n", buffer);
//         }
//     }
// }


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
    // uniqbasic();
}


// main function
// int main(int argc, char* argv[]){
//     if(argc<=1){
//         pipeFunction();
//         }
//     if(argc == 2){
//         init();
//         dataRead(argv[1]);
//         uniqbasic();
//     }
//     if(argc == 3){
//         if(strcmp(argv[1], "-d") == 0){
//              init();
//         dataRead(argv[2]);
//         uniqdfunction();
//         }
//         if(strcmp(argv[1], "-i") == 0){
//              init();
//             dataRead(argv[2]);
//             uniqifunction();
//         }
//         if(strcmp(argv[1], "-c") == 0){
//              init();
//         dataRead(argv[2]);
//         uniqcfunction();
//         }
//     if(argc > 3){
//         printf(1, "Too many arguments given");
//     }
//     }
//    exit();
// }

int 
main(int argc, char* argv[]) {

    char *flag = "basic";
    init();
    dataRead(argv[1]);
    
    char *arr_of_str[20];
    
    for(int i = 0; i< 20; i++){
        arr_of_str[i] = arr[i];
    }
    printf(1, "The length is :%d\n", length);
    for(int i=0; i < length; i++){
        printf(1,"");
    }
    uniq(flag, arr_of_str, length);
    exit();
    return 0;
 }