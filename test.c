#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"


int main(int argc, char *argv[]) {
    int pid;
    pid = fork();
    if(pid < 0)
    {
        printf(1, "The Fork has been failed!\n");
        exit();
    } 
    else if(pid > 0)
    {
        int creation_time=3, end_time=4, total_time=5;
        if (getprocstats(&creation_time, &end_time, &total_time) < 0) {
            printf(2, "Failed to get process times for PID %d\n", pid);
        } else {
            printf(1, "creation_time : %d ms\n", creation_time);
            printf(1, "end_time : %d ms\n", end_time);
            printf(1, "total_time : %d ms\n", total_time);
        }
    } 
    else 
    {
        if(argc < 2){
            printf(1, "test: Invalid number of arguments.\n");
            exit();
        }
        exec(argv[1], &argv[1]);
        exit();
    } 
    exit();
}