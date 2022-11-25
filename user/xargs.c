#include"kernel/types.h"
#include"kernel/stat.h"
#include"user/user.h"

int readLine(int fd,char *buff,int size){
    int pos =  0;
    for(int i = 0;i<size;i++){
        int len = read(STDIN,buff + pos,sizeof(buff));
        if(len == 0|| len == -1){
            return pos;
        }
        if(pos >= size) {
            return size;
        }
        pos += len;
        if(buff[pos-1] == '\n') 
        {
            buff[pos-1] = '\0';
            return pos;
        }
    }
    return pos;
}

int main(int argc,char ** argv){
    char buff[12];
    char* sargs[12];
    int idx  =0;
    if(argc < 2){
        fprintf(STDERR,"args should big 2");
        return 0;
    }
    for(int i = 1;i<argc;i++){
        sargs[idx] = malloc(strlen(argv[i]));
        memcpy(sargs[idx],argv[i],strlen(argv[i]));
        idx++;
    }
    int len;
    while((len = readLine(STDIN,buff,512))){
        sargs[idx] = malloc(len);
        memcpy(sargs[idx],buff,len);
        idx++;
    }
    int pid = fork();
    if(!pid){
        exec(argv[1],sargs);
    }else{
        wait(0);
    }
    return 0;
}