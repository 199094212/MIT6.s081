#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

alarmhandler_t sigalarm(int n,alarmhandler_t alarmhandler){
    struct proc* p  = myproc();
    alarmhandler_t old = p->sigalarm->sighandler;
    p->sigalarm->sighandler = alarmhandler;
    p->sigalarm->last_tick = ticks;
    p->sigalarm->interval_tick = n;
    p->sigalarm->running = 0;
    return old;
}

int sigreturn(void){
    struct proc* p  = myproc();
    if(p->sigalarm->running == 1){
        memmove(p->trapframe,&(p->sigalarm->trapframe),sizeof(*p->trapframe));
        p->sigalarm->last_tick = ticks;
        p->sigalarm->running = 0;
    }
    return 0;
} 