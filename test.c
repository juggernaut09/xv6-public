#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char *argv[]){
    if(argc < 2){
        printf(1, "test: Invalid number of arguments.\n");
        exit();
    }

    char *passing_argv[argc-1];
    for(int i=1; i < argc; i++){
        passing_argv[i-1] = argv[i];
    }

    test_changer();
    exec(passing_argv[0], passing_argv);
    exit();
}