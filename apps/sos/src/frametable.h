#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

//#include <types.h>
#include <cspace/cspace.h>


typedef struct frNode{
    uint32_t index;
    struct frNode * next;
} freeNode;

typedef struct {
    seL4_Word p_addr;
    seL4_CPtr cptr;
    freeNode * fNode;
} frame;

void frametable_init(seL4_Word low, seL4_Word high, cspace_t *cur_cspace);
seL4_Word frame_alloc(void);
int frame_free(seL4_Word v_addr);
 

void freeList_init(seL4_Word count);
uint32_t nextFreeFrame(freeNode ** fNode);
void freeList_freeFrame(freeNode * fNode); 

#endif
