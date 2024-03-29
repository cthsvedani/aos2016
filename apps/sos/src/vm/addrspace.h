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
#define	HEAP_BUFFER		 0x00010000

#define PAGE_OFFSET(a) ((a) & ((1 << seL4_PageBits) - 1))
#define PAGE_ALIGN(a) ((a) & ~((1 << seL4_PageBits) -1))

#define VM_FAULT_READ   0x1
#define VM_FAULT_WRITE  0x2
#define VM_FAULT_READONLY   0x4

typedef struct region_t {
    seL4_Word vbase;
    seL4_Word size;
    seL4_Word flags;
    struct region_t *next;
} region;

typedef struct shared_region_t {
    seL4_Word user_addr;
    seL4_Word vbase;
    seL4_Word size;
    seL4_Word flags;
    struct shared_region_t *next;
} shared_region;

typedef struct sos_PageTable{
	uint32_t frameIndex[VM_PTABLE_LENGTH];
}pageTable;

typedef struct sos_PageDirectory {
    region *regions;
    seL4_ARM_PageDirectory PageD;			//CPtr for seL4 Free
	seL4_Word PD_addr;						//UT * for ut_free
	pageTable * pTables[VM_PDIR_LENGTH];	//SOS Pagetable
    //may move to pagetable
	seL4_Word pTables_pAddr[VM_PDIR_LENGTH];			//seL4 Memory location
	seL4_ARM_PageTable pTables_CPtr[VM_PDIR_LENGTH];	//seL4 Cap Pointer
} pageDirectory;

pageDirectory* pageTable_create(void);

void PD_destroy(pageDirectory * pd);
void PT_destroy(pageTable * pt);

region * new_region(pageDirectory * pd, seL4_Word start,
		size_t len, seL4_Word flags);

region * find_region(pageDirectory * pd, seL4_Word vAddr);

void free_region_list(region* head);
void free_shared_region_list(shared_region * head);
void free_buffer(char* buf, int count);

int vm_fault(pageDirectory * pd,seL4_Word addr);

shared_region * get_shared_region(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd);
seL4_Word get_user_translation(seL4_Word user_vaddr, pageDirectory * user_pd);
int pt_ckptr(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd);
void get_shared_buffer(shared_region *shared_region, size_t count, char *buf);
void free_shared_buffer(char * buf, size_t count);
void put_to_shared_region(shared_region *shared_region, char *buf);

#endif
