#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

#include <cspace/cspace.h>
#define VMEM_START 0x20000000

typedef struct frNode{
    uint32_t index;
    struct frNode * next;
} freeNode;

typedef struct {
    //maybe not needed
    seL4_Word p_addr;
    seL4_CPtr cptr;
    freeNode * fNode;
} frame;

frame* ftable;

void frametable_init(seL4_Word low, seL4_Word high, cspace_t *cur_cspace);
uint32_t frame_alloc(void);
int frame_free(uint32_t index);
 

void freeList_init(seL4_Word count);
freeNode * nextFreeFrame(void);
void freeList_freeFrame(freeNode * fNode); 

#endif
