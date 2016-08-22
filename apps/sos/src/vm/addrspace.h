#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include "sel4/types.h"
#include <cspace/cspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define VM_PDIR_LENGTH		4096
#define VM_PTABLE_LENGTH	 256

#define REGION_STACK	 0x10000000
#define STACK_BOTTOM	 0x60000000
#define PROCESS_BREAK	 0x50000000
#define	HEAP_START		 0x10000000


#define VM_FAULT_READ   0x1
#define VM_FAULT_WRITE  0x2
#define VM_FAULT_READONLY   0x4

typedef struct region_t {
    seL4_Word vbase;
    seL4_Word size;
    seL4_Word flags;
    struct region_t *next;
} region;

typedef struct sos_PageTable{
	uint32_t frameIndex[VM_PTABLE_LENGTH];
}pageTable;

typedef struct sos_PageDirectory {
    region *regions;
    seL4_ARM_PageDirectory PageD;			//CPtr for seL4 Free
	seL4_Word PD_addr;						//UT * for ut_free
	pageTable * pTables[VM_PDIR_LENGTH];	//SOS Pagetable
	seL4_Word pTables_pAddr[VM_PDIR_LENGTH];			//seL4 Memory location
	seL4_ARM_PageTable pTables_CPtr[VM_PDIR_LENGTH];	//seL4 Cap Pointer
} pageDirectory;

pageDirectory* pageTable_create(void);

void PD_destroy(pageDirectory * pd);
void PT_destroy(pageTable * pt);

int new_region(pageDirectory * pd, seL4_Word start,
		size_t len, seL4_Word flags);

region * find_region(pageDirectory * pd, seL4_Word vAddr);

void free_region_list(region* head);

int vm_fault(pageDirectory * pd,seL4_Word addr);
seL4_Word get_translation(seL4_Word addr, seL4_Word faulttype, pageDirectory *pd);
int PT_ckflags(seL4_Word faulttype, seL4_Word addr, pageDirectory *pd);

#endif
