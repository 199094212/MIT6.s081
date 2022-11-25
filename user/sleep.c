#include "kernel/types.h"
#include"user/user.h"
#include"kernel/syscall.h"
#define stderr 2
int main(int argc,char**argv){
    if(argc <= 1){
        fprintf(stderr,"please input a number\n");
        exit(-1);
    }
    int time = atoi(argv[1]);
    if(time <= 0){
        fprintf(stderr,"please input a number\n");
        exit(-1);
    }
    sleep(time);
    return 0;
}