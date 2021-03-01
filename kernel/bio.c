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

#define NBUCKET 13
char buf[NBUCKET][20];

struct {
  struct buf head[NBUCKET];
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  struct spinlock steal_lock;
} bcache;

uint
ihash(uint blockno) {
  return blockno % NBUCKET;
}


void
binit(void)
{
  struct buf *b; 
  for (int i = 0; i < NBUCKET; i++) {
    snprintf(buf[i], 20, "bcache.bucket%d", i);
    initlock(&bcache.lock[i], (char*)buf[i]);
  }
  initlock(&bcache.steal_lock, "bcache");

  for (int i = 0; i < NBUCKET; i++) {
    // create a circular linked list
    // head.next is the first elem
    // head.prev is the last(LRU) elem
    struct buf *head = &bcache.head[i];
    head->prev = head;
    head->next = head;
  }
  int i;
  // Average distribut buf to each bucket
  for (b = bcache.buf, i = 0; b < bcache.buf + NBUF; b++, i = (i + 1) % NBUCKET) {
    b->next = bcache.head[i].next;
    b->prev = &bcache.head[i];
    bcache.head[i].next->prev = b;
    bcache.head[i].next = b;
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

  uint idx = ihash(blockno);

  acquire(&bcache.lock[idx]);
  for (b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached, find LRU
  for (b = bcache.head[idx].prev; b != &bcache.head[idx]; b = b->prev) {
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[idx]);


  acquire(&bcache.steal_lock);
  acquire(&bcache.lock[idx]);
  
    for (b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next) {
    if(b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.lock[idx]);
      release(&bcache.steal_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached, find LRU
  for (b = bcache.head[idx].prev; b != &bcache.head[idx]; b = b->prev) {
    if (b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[idx]);
      release(&bcache.steal_lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // steal from other bucket
  uint _idx = idx;
  idx = ihash(idx + 1);
  while (idx != _idx) {      

    acquire(&bcache.lock[idx]);
    // Not cached; recycle an unused buffer.
    for (b = bcache.head[idx].prev; b != &bcache.head[idx]; b = b->prev) {
      if (b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->prev->next = b->next;
        b->next->prev = b->prev;
        release(&bcache.lock[idx]);
        b->next = bcache.head[_idx].next;
        b->prev = &bcache.head[_idx];
        b->next->prev = b;
        b->prev->next = b;
        release(&bcache.lock[_idx]);
        release(&bcache.steal_lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[idx]);
    idx = ihash(idx + 1);
  }
  release(&bcache.lock[_idx]);
  release(&bcache.steal_lock);

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

  uint idx = ihash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // move to head
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[idx].next;
    b->prev = &bcache.head[idx];
    bcache.head[idx].next->prev = b;
    bcache.head[idx].next = b;
  }
  
  release(&bcache.lock[idx]);
}

void
bpin(struct buf *b) {
  uint idx = ihash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt++;
  release(&bcache.lock[idx]);
}

void
bunpin(struct buf *b) {
  uint idx = ihash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  release(&bcache.lock[idx]);
}


