#include "frametable.h"
#include "ut_manager/ut.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mapping.h>
#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

extern frame* ftable;

static frame * freeList;
static int _ftInit = 0;
static seL4_CPtr pd;
static int bootstrapFrames;

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

    dprintf(0,"Frametable Initialised with %d frames.\n", count);
   
    ftable = (frame*) VMEM_START;

    freeList = &ftable[bootstrapFrames + 1];

    freeList_init(count);

    _ftInit = 1;
}

void freeList_init(seL4_Word count){
	frameTop = count - 1;
	frameBot = bootstrapFrames + 1;
    for(int i = bootstrapFrames + 1; i < count-1; i++){
        ftable[i].index = i;
        ftable[i].next = &ftable[i + 1];
    }

    
}
/*
 * The physical memory is reserved via the ut_alloc, the memory is retyped into a frame, and the frame is mapped into the SOS
 * window at a fixed offset of the physical address.
 */
uint32_t frame_alloc(void) {
    frame* fNode = nextFreeFrame();
    if(!fNode){
		dprintf(0,"Next Frame Not Found\n");
		return 0;
	}

    int index = fNode->index;

    ftable[index].next = NULL;
    ftable[index].p_addr = ut_alloc(seL4_PageBits);
    if(!ftable[index].p_addr){
		dprintf(0,"Ut alloc failed\n");
		freeList_freeFrame(fNode);	
		return 0;
	} 

	int err;
    
   //retype memory and save cap for user  
    err = cspace_ut_retype_addr(ftable[index].p_addr, seL4_ARM_SmallPageObject, seL4_PageBits,
		cur_cspace,&(ftable[index].cptr));
    if(err){
		dprintf(0,"Retype Failed at index %d with error code %d\n", index, err);
		ut_free(ftable[index].p_addr, seL4_PageBits);
		ftable[index].p_addr = 0;
		freeList_freeFrame(fNode);
		return 0;
	}

   //save cap for kernel
    ftable[index].kern_cptr = cspace_copy_cap(cur_cspace, cur_cspace, ftable[index].cptr, seL4_AllRights);

    if(!ftable[index].kern_cptr){
		dprintf(0,"Kernel cap failed at index %d with error code %d\n", index, err);
		ut_free(ftable[index].p_addr, seL4_PageBits);
		ftable[index].p_addr = 0;
		freeList_freeFrame(fNode);
		return 0;
	}

    map_page(ftable[index].kern_cptr, seL4_CapInitThreadPD, 
            VMEM_START + (index << seL4_PageBits), 
            seL4_AllRights, seL4_ARM_Default_VMAttributes);

    return index;
}

/*
 * The physical memory is no longer mapped in the window, the frame object is destroyed, and the physical memory ranged
 * is returned via ut_free
 */
int frame_free(uint32_t index) {
    if(ftable[index].next == NULL){
	    freeList_freeFrame(&ftable[index]);
	    seL4_ARM_Page_Unmap(ftable[index].cptr);
	    cspace_delete_cap(cur_cspace, ftable[index].cptr);
	    ut_free(ftable[index].p_addr, seL4_PageBits);
	    ftable[index].cptr = (seL4_CPtr) NULL;
	    ftable[index].p_addr = (seL4_Word) NULL;
    }
    else return -1;

    return 0;
}

frame * nextFreeFrame( void ){
    frame* fNode;
    if(freeList){       
        fNode = freeList;
        freeList = freeList->next;
        return fNode;
    }
    return NULL;
}

void freeList_freeFrame(frame * fNode){
     fNode->next = freeList;
     freeList = fNode;
}

/*int flush_frame() {*/
/*}*/

/*int get_flush_candidate(){*/
/*}*/

