#include "types.h"
#include "user.h"
#include "syscall.h"


#define NULL ((void*)0)

int main()
{
    
    int *my_address = NULL;
    *my_address = 20;
    printf(1, "Accessing the value at Null Pointer : %d\n", *my_address);
    exit();
}

