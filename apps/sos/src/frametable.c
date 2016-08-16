#include "frametable.h"
#include "ut_manager/ut.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mapping.h>
#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

frame* ftable;
freeNode* freeList;
int _ftInit = 0;
seL4_CPtr pd;
int bootstrapFrames;

void frametable_init(seL4_Word low, seL4_Word high, cspace_t *cur_cspace) { 
    assert(!_ftInit);
	pd = seL4_CapInitThreadPD;
	seL4_Word size = high - low;
    seL4_Word count = size >> seL4_PageBits;
	int err;
    seL4_CPtr tmp;
	bootstrapFrames = (count*sizeof(frame) >> seL4_PageBits) + 1;

    for(int i = 0; i < bootstrapFrames; i++){
		seL4_CPtr f = ut_alloc(seL4_PageBits);
		err = cspace_ut_retype_addr(f, seL4_ARM_SmallPageObject, seL4_PageBits, cur_cspace, &tmp);
		conditional_panic(err,"Failed to allocate memory for frame table!\n");
		map_page(tmp, pd, (VMEM_START + (i << seL4_PageBits)), seL4_AllRights, seL4_ARM_PageCacheable);       
	}
   
    ftable = (frame*) VMEM_START;
    freeList_init(count); 

    _ftInit = 1;
}

void freeList_init(seL4_Word count) {
    freeList = malloc(sizeof(freeNode));
    freeList->index = bootstrapFrames;
    freeList->next = NULL;
    freeNode* head = freeList;
    for(int i = bootstrapFrames + 1; i < count; i++){
        head->next = malloc(sizeof(freeNode));
        head = head->next;
        head->index = i;
        head->next = NULL;
    }
}

/*
 * The physical memory is reserved via the ut_alloc, the memory is retyped into a frame, and the frame is mapped into the SOS
 * window at a fixed offset of the physical address.
 */
uint32_t frame_alloc(seL4_Word * vaddr) {
    freeNode* fNode = nextFreeFrame();
    if(!fNode){
        *vaddr = 0;
		return 0;
	}

    int index = fNode->index;

    ftable[index].fNode = fNode;
    ftable[index].p_addr = ut_alloc(seL4_PageBits);
    if(!ftable[index].p_addr){
        *vaddr = 0;
		freeList_freeFrame(index);
		ftable[index].fNode = NULL;
		return 0;
	} 

	int err;

    err = cspace_ut_retype_addr(ftable[index].p_addr, seL4_ARM_SmallPageObject, seL4_PageBits,
		cur_cspace,&(ftable[index].cptr));
    if(err){
		ut_free(ftable[index].p_addr, seL4_PageBits);
		ftable[index].p_addr = NULL;
		freeList_freeFrame(index);
		ftable[index].fNode = NULL;
        *vaddr = 0;
		return 0;
	}

    *vaddr = VMEM_START + (index << seL4_PageBits); 
    err = map_page(ftable[index].cptr, pd, *vaddr, seL4_AllRights, seL4_ARM_PageCacheable); 
	if(err){
		cspace_delete_cap(cur_cspace, ftable[index].cptr);
		ftable[index].cptr = NULL;
		ut_free(ftable[index].p_addr, seL4_PageBits);
		ftable[index].p_addr = NULL;
		freeList_freeFrame(index);
		ftable[index].fNode = NULL;
        *vaddr = 0;
		return 0;
	}

    return index;
}

/*
 * The physical memory is no longer mapped in the window, the frame object is destroyed, and the physical memory ranged
 * is returned via ut_free
 */
int frame_free(uint32_t index) {
    if(ftable[index].fNode){
	    freeList_freeFrame(ftable[index].fNode);
	    seL4_ARM_Page_Unmap(ftable[index].cptr);
	    cspace_delete_cap(cur_cspace, ftable[index].cptr);
	    ut_free(ftable[index].p_addr, seL4_PageBits);
	    ftable[index].cptr = (seL4_CPtr) NULL;
	    ftable[index].p_addr = (seL4_Word) NULL;
	    ftable[index].fNode = NULL;
    }
    else return -1;

    return 0;
}

freeNode * nextFreeFrame( void ){
    freeNode* fNode;
    if(freeList){       
        fNode = freeList;
        freeList = freeList->next;
        return fNode;
    }
    return NULL;
}

void freeList_freeFrame(freeNode * fNode){
     fNode->next = freeList;
     freeList = fNode;
}

//vspace is represented by a PD object
//PDs map page tables
//PTs map pages
//creating a PD (by retyping) creates the vspace
//deleting the PD deletes the vspace

//Code example:
//get untyped memory (a page size)
//seL4_Word frame_addr = ut_alloc(seL4_PageBits);

//retype memory to type seL4_ARM_Page
//cspace_ut_retype_addr(frame_addr, seL4_ARM_Page, seL4_ARM_Default_VMAttributes);

//insert mapping
//map_page(frame_cap, pd_cap, 0xA00000000, seL4_AllRights, seL4_ARM_Default_VMAttributes);

//zero the new memory
//bzero((void *)0xA0000000, PAGESIZE);

//each mapping requeires its own frame cap even for the same fram, and has:
//virtual_address, phys_address, address_space and frame_cap
//address_space struct identifies the level 1 page_directory cap
//you need to keep track of (frame_cap, PD_cap, v_adr, p_adr)

//
//sizes of memory objects
//
//OBJECT        Size in bytes (and alignment)
//Frame             2^12
//PageD             2^14
//PageT             2^10


