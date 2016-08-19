#ifndef _ADDRSPACE_H_
#define _ADDRSPACE_H_

#include "sel4/types.h"
#include <cspace/cspace.h>

#define VM_PDIR_LENGTH		4096
#define VM_PTABLE_LENGTH	2048

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

#endif
