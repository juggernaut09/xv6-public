#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

char *arr[1024];
int length = 0;

void init(){
    for(int i=0; i < 1024; i++){
        arr[i] = malloc(100);
    }
}

int fetch_fd(char *fname){
    if(strlen(fname)) return open(fname, O_RDONLY);
    else return -1;
}

void read_input(int fd){
    int num_of_bytes_read, index=0;
    char read_buffer[1024], str_buffer[1024];
    while((num_of_bytes_read = read(fd, read_buffer, sizeof(read_buffer))) != 0){
        for(int i = 0; i < num_of_bytes_read; i++){
            if(read_buffer[i] != '\n'){
                str_buffer[index++] = read_buffer[i];
            } else {
                strcpy(arr[length++], str_buffer);
                memset(str_buffer, '\0', 1024);
                index = 0;
            }
        }
    }
}

void head_single_file(char **arr_of_strs, int len, int n){
    if(len < n){
        for(int i=0; i< len; i++){
            printf(1, "%s\n", arr_of_strs[i]);
        }
    } else {
        if(len >= n){
            for(int i=0; i < n; i++){
                printf(1, "%s\n", arr_of_strs[i]);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    printf(1,  "Head command is getting executed in user mode.\n");
    int fd;
    init();
    if(argc <= 1){ 
        // head 
        fd = 0;
        read_input(fd);
        printf(1, "\n");
        head_single_file(arr, length, 14);
    }
    if(argc == 2){
        // head filename.txt
        fd = fetch_fd(argv[1]);
        if(fd < 0){
            printf(1, "head: cannot open '%s' for reading: No such file or directory\n", argv[3]);
            exit();        
        }
        read_input(fd);
        head_single_file(arr, length, 14);
    }
    if(argc > 2){
        int no_of_files, file_idx, n;
        if(strcmp(argv[1], "-n") == 0){
            no_of_files = argc - 3;
            file_idx = argc - no_of_files;
            n = atoi(argv[2]);   
        } else {
            no_of_files = argc - 1;
            file_idx = argc - no_of_files;
            n = 14;
        }
        if(no_of_files == 1){
            fd = fetch_fd(argv[file_idx]);
            if(fd < 0){
                printf(1, "head: cannot open '%s' for reading: No such file or directory\n", argv[file_idx]);
                exit();        
            }
            read_input(fd);
            head_single_file(arr, length, n);
        } else {
            for(int i = file_idx; i < argc; i++){
                fd = fetch_fd(argv[i]);
                if(fd < 0){
                    printf(1, "head: cannot open '%s' for reading: No such file or directory\n", argv[i]);
                    exit();        
                }
                printf(1, "==> %s <==\n", argv[i]);
                read_input(fd);
                head_single_file(arr, length, n);
                length = 0;
            }
                
        }
    }
    exit();
}