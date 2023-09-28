#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  int len;
  uint64 va, mask;
  if(argaddr(0, &va) < 0)
    return -1;
  if(argint(1, &len) < 0)
    return -1;
  if(argaddr(2, &mask) < 0)
    return -1;
  // if((va % PGSIZE) != 0){
  //   printf("sys_pgaccess: va %p\n", va);
  //   return -1;
  // }
  va = va - (va % PGSIZE);
  if(len < 0){
    printf("sys_pgaccess: len %d\n", len);
    return -1;
  }
  //printf("va -> %p, len -> %d, mask -> %p\n", va, len, mask);
  int buffer_len = (len % 8) ? (len / 8 + 1) : (len / 8);
  char buffer[1024];
  if(buffer_len > 1024){
    panic("sys_pgaccess : buffer_len too large\n");
  }
  memset(buffer, 0, buffer_len);
  //vmprint(myproc() -> pagetable);
  for(int i = 0; i < len; i ++){
    pte_t * pte = walk(myproc() -> pagetable, va + i * PGSIZE, 0);
    if(pte != 0 && (*pte) & PTE_A){
      (*pte) -= PTE_A;
      int n = len - i - 1;
      //printf("n -> %d\n", n);
      int idx = buffer_len - 1 - n / 8;
      buffer[idx] += (1 << (7 - n % 8));
    }
  }
  if(copyout(myproc() -> pagetable, mask, buffer, buffer_len) < 0){
    return -1;
  }

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
