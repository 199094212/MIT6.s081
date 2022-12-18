## traps实验:
#### xv6寄存器说明：

#### trap执行过程:
1、函数调用user:syscall

2、ecall

    1、切换CPU模式
    2、保存PC到寄存器sepc
    3、PC跳转到stvec)

3、进入uservec（保存寄存器)

4、usertrap（保存和恢复硬件状态,打开中断）

5、调用syscall

6、通过SYSCALLID调用真正的内核函数

7、usertrapret(关闭中断，恢复stvec)

8、uservecret(恢复保存的寄存器，并且回到用户态（sret))

    sret:
        set PC  =$sepc
        重新启用中断
#### backtrace实验
原理

    |return addr|(x)
    |   fp      |(x+8) | \[*(y-16)\]
    | save reg  |(&fp:y - 16)
    | local var |
    |return addr| (我们要的结果:y)
    |   fp      | 
    | save reg  |
    | local var |
    当前的s0 -->fp
    所以 x = *(fp - 8)
    而我们要的上一级的地址fp的栈地址为:y - 16
    此处存储了fp的栈的位置所以 last_fp: *(y-16)
    此外我们知道这些fp的值应该在一个页面当中，所以其地址为
    [lowpage,lowpage + 4096)

```c
//sysproc.c
sys_sleep(void)
{
  backtrace();
  int n;
  uint ticks0;
//...
}

//risv.h
static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x) );
  return x;
}

//printf.c
void backtrace(void){
  uint64 fp = r_fp(); // get s0
  printf("backtrace:\n");
  uint64 low = PGROUNDDOWN(fp);
  uint64 up = PGROUNDUP(fp);
  while(fp >= low && fp < up){
    printf("%p\n",*(uint64*)(fp-8));
    fp = *(uint64*)(fp -16);
  }
}
```

#### sigalarm实验
0、准备工作添加相关的makefile

1、根据提示需要实现sigalarm和sigreturn两个函数,实际上这边更好的方案是在用户层做一个封装，然后让他每次陷入用户层封装的函数，这样用户就不需要自己去调用sigreturn函数。
```c
/*user.h*/
typedef void (*alarmhandler_t)(void);
alarmhandler_t sigalarm(int n,alarmhandler_t);
int sigreturn(void);

/*usys.pl*/
entry("sigalarm");
entry("sigreturn");
```
2、创建sigalarm和xigreturn两个系统调用（实验二已经做过了，就不多说了）
```c
//def.h
typedef void (*alarmhandler_t)(void);
alarmhandler_t sigalarm(int,alarmhandler_t);
int sigreturn(void);

//syscall.h
#define SYS_sigalarm 22
#define SYS_sigreturn 23

//syscall.c
extern uint64 sys_sigalarm(void);
extern uint64 sys_sigreturn(void);

static uint64 (*syscalls[])(void) = {
[SYS_sigalarm] sys_sigalarm,
[SYS_sigreturn] sys_sigreturn,
}
//sysproc.c
uint64
sys_sigalarm(void)
{
  int n;
  uint64 func;
  argint(0,&n);
  argaddr(1,&func);
  return (uint64)sigalarm(n,(alarmhandler_t)(func));
}
uint64
sys_sigreturn(void)
{
  sigreturn();
  return 0;
}

// signal.c
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

alarmhandler_t sigalarm(int n,alarmhandler_t alarmhandler){
}

int sigreturn(void){
} 
```

3、因为是导入了一个回调函数，关于回调函数相关的信息需要保存到proc当中,并且对proc进行分配和回收（分配和回收可以查看pagtable实验)
```c
//proc.h
// 根据题目要求很容易得到下面结构体
struct sigalarm_task
{
  void (*sighandler)(void);  //回调函数
  int last_tick;             //上一次时间滴
  int interval_tick;         //间隔时间滴
};
struct proc {
    //....
    struct sigalarm_task* sigalarm;  //sigalarm相关结构体
    //...
}

//proc.c
allocproc(void){
//...
found:
    if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
    }
    if((p->sigalarm = (struct sigalarm_task*)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    }
//...
    p->sigalarm->interval_tick = 0;
    p->sigalarm->last_tick = 0;
    p->sigalarm->sighandler = (void*)0;
    return p;
}
//... 如果没有进行释放无法通过usertests
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->sigalarm)
    kfree((void*)p->sigalarm);
  p->sigalarm = 0;
//...
}
//...
```
4、sigalarm 实现sigalarm，这一部分就是去更新proc->sigalarm的参数
```c
//signal.c
alarmhandler_t sigalarm(int n,alarmhandler_t alarmhandler){
    struct proc* p  = myproc();
    alarmhandler_t old = p->sigalarm->sighandler;
    p->sigalarm->sighandler = alarmhandler;
    p->sigalarm->last_tick = ticks;
    p->sigalarm->interval_tick = n;
    return old;
}

5、目前来说一些sigalarm基本实现了，但是sigreturn我们并不知道需要实现什么，此外我们得找一个地方去执行sigalrm的回调函数，sigreturn目前来说暂时还没有找到更好的方法，我们先去找sigalrm的回调函数执行的位置。我们知道sighandler的代码处于用户区，而在内核态我们是无法执行用户态函数的，但是我们知道 当我们从内核态出去回到用户态的时候我们的下一条执行p->trapframe->epc的代码.
```c
//trap.c
if(which_dev ==2 && p->sigalarm->interval_tick != 0 ){
    if(ticks - p->sigalarm->last_tick > p->sigalarm->interval_tick && !p->sigalarm->running){
    p->trapframe->epc = (uint64)p->sigalarm->sighandler;
    }
}
```
6、虽然可以成功跳转，但是因为执行这一块之前的寄存器内容已经被改变，当我们从回调函数回来应该恢复回调函数，所以此处要，保存回调函数和重写回调函数(test1、test3)
```c
//proc.c
struct sigalarm_task
{
  void (*sighandler)(void); 
  int last_tick;
  int interval_tick;
  struct trapframe trapframe;
};
//trap.c
if(which_dev ==2 && p->sigalarm->interval_tick != 0 ){
    if(ticks - p->sigalarm->last_tick > p->sigalarm->interval_tick && !p->sigalarm->running){
        memmove(&(p->sigalarm->trapframe),p->trapframe,sizeof(*p->trapframe));
        p->trapframe->epc = (uint64)p->sigalarm->sighandler;
    }
}
```

7、我们发现test2()通过不了，是因为sigalarm的回调函数是可重入的，这就导致，在执行sigalarm的回调函数过程中还能再次调用sigalarm，这导致了结果不符合预期，所以我们通过需要增加一个running字段保证他是不可重入的
```c
//proc.c
struct sigalarm_task
{
  void (*sighandler)(void); 
  int last_tick;
  int interval_tick;
  struct trapframe trapframe;
  int running;
};
//trap.c
if(which_dev ==2 && p->sigalarm->interval_tick != 0 ){
    if(ticks - p->sigalarm->last_tick > p->sigalarm->interval_tick && !p->sigalarm->running){
        memmove(&(p->sigalarm->trapframe),p->trapframe,sizeof(*p->trapframe)); //保存最早的状态
        p->trapframe->epc = (uint64)p->sigalarm->sighandler;
        p->sigalarm->running = 1;
    }
}
```
8、此处呢我们是需要恢复到调用回调函数的状态，那么我们根据上面可以了解到sigretn需要回写寄存器状态，标记sigalarm的running结束以及更新上一次的时间滴
```c
//signal.c
int sigreturn(void){
    struct proc* p  = myproc();
    if(p->sigalarm->running == 1){
        memmove(p->trapframe,&(p->sigalarm->trapframe),sizeof(*p->trapframe));
        p->sigalarm->last_tick = ticks;
        p->sigalarm->running = 0;
    }
    return 0;
} 
```
9、最终的结果，核心代码
```c
//proc.h
struct sigalarm_task
{
  void (*sighandler)(void); 
  int last_tick;
  int interval_tick;
  struct trapframe trapframe;
  int running;
};

//signal.c
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

//trap.c
if(which_dev ==2 && p->sigalarm->interval_tick != 0 ){
    if(ticks - p->sigalarm->last_tick > p->sigalarm->interval_tick && !p->sigalarm->running){
        memmove(&(p->sigalarm->trapframe),p->trapframe,sizeof(*p->trapframe));
        p->trapframe->epc = (uint64)p->sigalarm->sighandler;
        p->sigalarm->running = 1;
    }
}
```