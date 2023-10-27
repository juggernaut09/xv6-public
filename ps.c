#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

int main(int argc, char *argv[]){
    if(argc == 1){
        ps();
    } else if (argc > 1){
        int isInteger = 1;  // Assume it's an integer by default
        char *value = argv[1];
        // Loop through each character in the value
        for (int i = 0; value[i] != '\0'; i++) {
            if (value[i] < '0' || value[i] > '9') {
                isInteger = 0;  // Not a digit, so it's not an integer
                break;
            }
        }
        if (isInteger) {
            ps_pid(atoi(argv[1]));
        } else {
            ps_pname(argv[1]);
        }

    }
    
    exit();
}