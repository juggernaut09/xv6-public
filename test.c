#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"


int main(int argc, char *argv[]) {
    int pid;
    pid = fork();
    // printf(1, "Before the condition : %d\n", pid);
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
            // printf(1, "Process %d: Creation Time: %d, End Time: %d, Total Time: %d\n",
            //         pid, pstats.creation_time, pstats.end_time, pstats.total_time);
            printf(1, "creation_time : %dms\n", creation_time);
            printf(1, "end_time : %dms\n", end_time);
            printf(1, "total_time : %dms\n", total_time);
        }
    } 
    else 
    {
        if(argc < 2){
            printf(1, "test: Invalid number of arguments.\n");
            exit();
        }
        char *passing_argv[argc-1];
        for(int i=1; i < argc; i++) passing_argv[i-1] = argv[i];
        exec(passing_argv[0], passing_argv);
        exit();
    } 
    exit();
}