#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <cspace/cspace.h>
#define VMEM_START 0x20000000

typedef struct frameNode{
    uint32_t index;
    seL4_Word p_addr;
    seL4_CPtr cptr;
    seL4_CPtr kern_cptr;
    struct frameNode * next;
} frame;

frame* ftable;

void frametable_init(seL4_Word low, seL4_Word high, cspace_t *cur_cspace);
uint32_t frame_alloc(void);
int frame_free(uint32_t index);
 

void freeList_init(seL4_Word count);
frame * nextFreeFrame(void);
void freeList_freeFrame(frame * fNode); 

#endif
