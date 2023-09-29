#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char *argv[]) {
    printf(1, "Venkat Teja Ravi.\n");
    int pid;
    pid = fork();
    // printf(1, "Before the condition : %d\n", pid);
    if(pid < 0){
        printf(1, "The Fork has been failed!\n");
        exit();
    } else if(pid == 0){
        if(argc < 2){
            printf(1, "test: Invalid number of arguments.\n");
            exit();
        }
        char *passing_argv[argc-1];
        for(int i=1; i < argc; i++){
            passing_argv[i-1] = argv[i];
        }
        exec(passing_argv[0], passing_argv);
        exit();
    } 
    printf(1, "The pid : %d \n", pid);
    // else{
    //     int cpid;
    //     cpid = wait();
    //     printf(1, "Inside the Parent Process : The child : %d\n", cpid);
    //     printf(1, "Inside the Parent Process : The child : %d\n", pid);
    // }
    exit();
}