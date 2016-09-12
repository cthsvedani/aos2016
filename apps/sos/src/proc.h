#ifndef _PROC_H_
#define _PROC_H_
#include "vm/addrspace.h"
#include "fdtable.h"

struct proc {
    seL4_Word tcb_addr;
    seL4_TCB tcb_cap;
	region * heap;
	pageDirectory * pd;
    seL4_CPtr ipc_buffer_cap;
	uint32_t ipc_frame;
	fdnode fdtable[MAX_FILES];
    int swapfile;
    cspace_t *croot;
};

#endif
