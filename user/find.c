#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
const char* filename(const char *filepath,char * buff){
    int len = strlen(filepath);
    int idx =0;
    for(int i =0;i<len;i++){
        if(filepath[i] == '/'){
            idx = 0;
        }else{
            buff[idx] = filepath[i];
            idx++;
        }
    }
    return buff;
}
void findFile(char* path,char *cmp){
    int fd = open(path,0);
    char *p;
    struct dirent de;
    if(fd < 0){
        fprintf(STDERR, "find: cannot open %s\n", path);
        return ;
    }
    struct stat st;
    if(fstat(fd,&st) < 0){
        fprintf(2, "find: cannot stat %s\n",path);
        close(fd);
        return ;
    }
    char buf[512] ="";
    switch (st.type)
    {
        case T_DEVICE:
        case T_FILE:
            if(strcmp(filename(path,buf),cmp) == 0){
                fprintf(STDOUT,"%s\n",path);
            }
            break;
        case T_DIR:{
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                if(strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0)
                    continue;
                fprintf(STDERR,"%s\n",de.name);
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                findFile(buf,cmp);
            }
            break;
        }
    default:
        break;
    }
    close(fd);
}
int main(int argc,char** argv){

    if(argc <= 2){
        fprintf(STDERR, "find: args not null\n");
        return 0;
    }   
    findFile(argv[1],argv[2]);
    return 0;
}