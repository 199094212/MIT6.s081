#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int parther_proc(int rfd,int wfd);
void child_proc(int *p){
    close(p[1]);
    int pip[2];
    pipe(pip);
    int ret = parther_proc(p[0],pip[1]);
    if(!ret){
        if(!fork()){
            child_proc(pip);
        }else{
            close(pip[0]);
            close(pip[1]);
            close(p[0]);
            wait(0);
        }
    }
    return ;
}
int parther_proc(int rfd,int wfd){
    int n;
    int s;
    int ret = read(rfd,&n,sizeof(int));
    if(ret == 0){
        return -1;
    }
    fprintf(STDOUT,"prime %d\n",n);
    int len;
    while(1){
        len = read(rfd,&s,sizeof(int));
        if(len == 0 || len == -1){
            break;
        }
        if(s % n == 0)
            continue;
        else{
            write(wfd,&s, sizeof(int));
        }  
    }
    return 0;
}
int main(){
    int i,p[2];
    pipe(p);
    // 首个父进程 想子进程写入数据
    for(i = 2;i<=35;i++){
        write(p[1],&i,sizeof(int));
    }
    int pid = fork();
    if(!pid){
        child_proc(p);
    }else{
        close(p[0]);
        close(p[1]);
        wait(0);
    }
    return 0;
}