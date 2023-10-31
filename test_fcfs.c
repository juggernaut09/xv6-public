#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

char* strdup(const char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }

    char* new_str = (char*)malloc(len + 1); // +1 for the null terminator
    if (new_str == 0) {
        return 0; // Memory allocation failed
    }

    for (int i = 0; i <= len; i++) {
        new_str[i] = str[i];
    }

    return new_str;
}

#define MAX_TOKENS 10  // Maximum number of tokens

void tokenize(char *str, char *tokens[], int *count) {
    *count = 0;
    char *token = str;
    while (*str != '\0' && *count < MAX_TOKENS) {
        if (*str == ' ') {
            *str = '\0'; // Replace space with null terminator
            tokens[(*count)++] = token;
            token = str + 1; // Move to the next character
        }
        str++;
    }
    // Don't forget the last token if it's not followed by a space
    if (token != str) {
        tokens[(*count)++] = token;
    }
}

// Custom function to copy a string until a delimiter is found
void custom_strcpy_until(char *destination, char *source, char delimiter) {
    for(int i =0; i< strlen(source); i++ ){
        if(source[i] && source[i] != delimiter){
            destination[i] = source[i];
        } else {
            destination[i] = ' ';
        }
    }
}

int main(int argc, char *argv[]) {
    char **processes = (char **)malloc(argc * sizeof(char *));

    int num_of_procs = 0;  // Count of user processes
    for (int i = 1; i < argc; i++) {
        // Iterate through each argument in argv
        char process_name[64] = "";

        // Split the argument based on commas and replace ',' with space
        custom_strcpy_until(process_name, argv[i], ',');
        
        // Check if the argument contains "_user" or "_kernel" and store accordingly
        
        processes[num_of_procs] = (char *)malloc(strlen(process_name) + 1);
        strcpy(processes[num_of_procs], process_name);
        num_of_procs++;
    }

    // CHange the scheduler_type to 1 // FCFS
    if(set_scheduler(1) < 0) exit(); else printf(1, "The Scheduler is Set to FCFS.\n");

    // Print user processes
    // printf(1, "The Processes:\n");
    for (int i = 0; i < num_of_procs; i++) {
        char *inputString = processes[i];
        char *tokens[MAX_TOKENS];
        int count;
        // printf(1, "input String: %s\n", processes[i]);
        tokenize(inputString, tokens, &count);
        int pid = fork();
        if (pid < 0) {
            printf(1, "Fork failed\n");
            exit();
        }
       if (pid == 0) {
            // This is the child process
            char *new_argv[count];
            for (int i = 0; i < count; i++) {
                // printf(1, "Token %d: %s\n", i, tokens[i]);

                // Create a new copy of the token
                new_argv[i] = strdup(tokens[i]);

                if (new_argv[i] == 0) {
                    printf(1, "Failed to allocate memory for new_argv[%d]\n", i);
                    // Handle the error appropriately
                    // You may want to free memory here if needed
                    exit();
                }
            }

            // Make sure the last element is NULL
            new_argv[count] = 0;
            
            if (exec(new_argv[0], new_argv) < 0) {
                printf(1, "Handle the error, e.g., print an error message\n");
            }
            

            exit();
        }
    }
    for (int i = 0; i < num_of_procs; i++) {
        int creation_time=3, end_time=4, total_time=5, wtime=6, rtime=7;
        int pid = getprocstats(&creation_time, &end_time, &total_time, &wtime, &rtime);
        if ( pid < 0) {
            printf(2, "Failed to get process times for PID %d\n", pid);
        } else {
            printf(1, "creation_time : %d ms\n", creation_time);
            printf(1, "end_time : %d ms\n", end_time);
            printf(1, "total_time : %d ms\n", total_time);
            printf(1, "wtime : %d ms\n", wtime);
            printf(1, "rtime : %d ms\n", rtime);
        }
    }

    for(int i = 0; i< num_of_procs; i++){
        free(processes[i]);
    }
    free(processes);
    exit();
}
