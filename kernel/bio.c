// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASHCOUNT 293

struct hashentry{
  struct buf head;
  struct spinlock lock;
};
struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  int new;
  struct hashentry hash[HASHCOUNT];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  bcache.new = 0;
  for(int i = 0; i < HASHCOUNT; i ++){
    initlock(&bcache.hash[i].lock, "bcache");
    bcache.hash[i].head.prev = &bcache.hash[i].head;
    bcache.hash[i].head.next = &bcache.hash[i].head;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;


  int key = blockno % HASHCOUNT;
  acquire(&bcache.hash[key].lock);
  for(b = bcache.hash[key].head.next; b != &bcache.hash[key].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hash[key].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.hash[key].lock);
  b = 0;
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  if(bcache.new != NBUF){
    acquire(&bcache.lock);
    b = bcache.buf + bcache.new;
    bcache.new ++;

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    acquire(&bcache.hash[key].lock);

    b->next = bcache.hash[key].head.next;
    b->prev = &bcache.hash[key].head;
    bcache.hash[key].head.next->prev = b;
    bcache.hash[key].head.next = b;

    release(&bcache.hash[key].lock);

    release(&bcache.lock);
    acquiresleep(&b->lock);
    return b;
  }else{
    uint mintick = ~0u;

    struct spinlock* hashentrylock = 0;
    for(int i = 0; i < HASHCOUNT; i ++){
      acquire(&bcache.hash[i].lock);
      for(struct buf *ib = bcache.hash[i].head.next; ib != &bcache.hash[i].head; ib = ib->next){
        if(ib -> refcnt == 0 && (ib -> tick) <= mintick){
          mintick = ib -> tick;
          b = ib;
          if(hashentrylock)
            release(hashentrylock);
          hashentrylock = &bcache.hash[i].lock;
        }
      }
      if(hashentrylock != &bcache.hash[i].lock)
        release(&bcache.hash[i].lock);
    }
    if(b){
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;

      b->next->prev = b->prev;
      b->prev->next = b->next;

      if(hashentrylock)
        release(hashentrylock);
      else
        panic("hashentrylock == 0");
      
      acquire(&bcache.hash[key].lock);
      b->next = bcache.hash[key].head.next;
      b->prev = &bcache.hash[key].head;
      bcache.hash[key].head.next->prev = b;
      bcache.hash[key].head.next = b;
      release(&bcache.hash[key].lock);

      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int key = b ->blockno % HASHCOUNT;
  acquire(&bcache.hash[key].lock);
  b->refcnt--;
  acquire(&tickslock);
  b -> tick = ticks;
  release(&tickslock);
  release(&bcache.hash[key].lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


