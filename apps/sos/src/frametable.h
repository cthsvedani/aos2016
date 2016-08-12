#ifndef _FRAMETABLE_H_
#define _FRAMETABLE_H_

//#include <types.h>
#include <cspace/cspace.h>

typedef struct {
    seL4_Word p_addr;
    seL4_CPtr cptr;
} frame;

typedef struct frNode{
    seL4_Word p_addr;
    uint32_t index;
    struct frNode * next;
} freeNode;


 
void frametable_init(selL4_Word low, seL4_Word high);

void freeList_init(seL4_Word count);
#endif
