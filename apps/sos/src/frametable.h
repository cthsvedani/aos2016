#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <cspace/cspace.h>
#include "vm/addrspace.h"
#define VMEM_START 0x20000000

typedef struct frameNode{
    pageTableEntry *pte;
    uint32_t index;
	uint32_t backingIndex;
    seL4_Word p_addr;
    seL4_CPtr cptr;
    seL4_CPtr kern_cptr;
    struct frameNode * next;
	int pinned;
} frame;
int frameTop;
int frameBot;
frame* ftable;
void debug_frametable();

void frametable_init(seL4_Word low, seL4_Word high, cspace_t *cur_cspace);
uint32_t frame_alloc(void);
int frame_free(uint32_t index);

void pin_frame(uint32_t index); 
void pin_frame_kvaddr(uint32_t kvaddr);
void unpin_frame_kvaddr(uint32_t kvaddr);

void freeList_init(seL4_Word count);
frame * nextFreeFrame(void);
void freeList_freeFrame(frame * fNode); 

//swapping 
int flush_frame();
int get_flush_candidate();

#endif
