#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char** argv){
    int pips[2];
    int spips[2];
    pipe(pips);
    pipe(spips);
    int pid = fork();
    if (!pid){  // 子进程
        char byte;
        close(spips[0]);
        close(pips[1]);
        write(spips[1],"b",1);
        read(pips[0],&byte,1);
        fprintf(STDOUT,"%d: received ping\n",getpid());
    }else{
        char byte;
        close(spips[1]);
        close(pips[0]);
        write(pips[1],"a",1);
        wait(0);
        read(spips[0],&byte,1);
        fprintf(STDOUT,"%d: received pong\n",getpid());
    }
    return 0;
}