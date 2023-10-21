// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void
kinit()
{
  for(int i = 0; i < NCPU; i ++)
    initlock(&kmem[i].lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *pa;
  pa = (char*)PGROUNDUP((uint64)pa_start);
  uint64 n = 0;
  for(; pa + PGSIZE <= (char*)pa_end; pa += PGSIZE){
    struct run *r;

    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
      panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem[n % NCPU].lock);
    r->next = kmem[n % NCPU].freelist;
    kmem[n % NCPU].freelist = r;
    release(&kmem[n % NCPU].lock);
    n ++;
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int n = cpuid();
  pop_off();
  if(n >= NCPU)
    panic("n >= NCPU");
  acquire(&kmem[n].lock);
  r->next = kmem[n].freelist;
  kmem[n].freelist = r;
  release(&kmem[n].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int n = cpuid();
  pop_off();
  if(n >= NCPU)
    panic("n >= NCPU");
  acquire(&kmem[n].lock);
  r = kmem[n].freelist;
  if(r){
    kmem[n].freelist = r->next;
    release(&kmem[n].lock);
  }
  else{
    release(&kmem[n].lock);
    
    for(int i = n + 1; i != n; (i == NCPU - 1) ? (i = 0) : (i ++)){
      acquire(&kmem[i].lock);
      r = kmem[i].freelist;
      if(r)
        kmem[i].freelist = r->next;
      release(&kmem[i].lock);
      if(r)
        break;
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
