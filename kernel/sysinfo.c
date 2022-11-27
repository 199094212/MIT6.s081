#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "sysinfo.h"
extern uint64 frre_mem(void);
extern uint64 num_proc(void);
int sysinfo(uint64 addr);

uint64
sys_info(void){
    uint64 addr;
    argaddr(0,&addr);
    return sysinfo(addr);
}

int
sysinfo(uint64 addr){
    /*检查内存合法*/
    struct proc *p = myproc();
    struct sysinfo info;
    // 填充proc
    info.freemem = frre_mem();
    // 填充freemem
    info.nproc = num_proc();
    if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0){
        return -1;
    }
    return 0;
}