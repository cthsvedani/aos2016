#include "addrspace.h"

#include "ut_manager/ut.h"
#include "frametable.h"

#define verbose 5
#include "sys/debug.h"
#include "sys/panic.h"


pageDirectory* pageTable_create(void){
	//Create a new Page Directory
	pageDirectory* pd = malloc(sizeof(pageDirectory));
	pd->PD_addr = ut_alloc(seL4_PageDirBits);
	int err = cspace_ut_retype_addr(pd->PD_addr, seL4_ARM_PageDirectoryObject,
		seL4_PageDirBits, cur_cspace, &(pd->PageD));

	if(err){
		dprintf(0,"Could not retype memory in pageTable_create!\n");
	}
	
	pd->regions = NULL;

	return pd;
}

int vm_fault(seL4_Word addr) {
    //check valid faulttype
    
    return -1;
}

seL4_Word get_translation(seL4_Word addr, seL4_Word faulttype, pageDirectory *pd) {
    uint_32t pd_offset = addr >> 20; //top 12 bits
    uint_32t pt_offset = addr << 10;
    pt_offset = pt_offset >> 22; //next 10 bits

    if(!pd->pTables[pd_offset]) { //missing level 1 entry 
        //check flags
        //create lvl 1 entry
        //return new paddr
    } else if(!pd->pTables[pd_offset]->frameIndex[pt_offset]) { //missing level 2 entry
        //check flags
        //create lvl 2 entry
        //return new paddr
    }
    return pd->pTables[pd_offset]->frameIndex[pt_offset];
}

int PT_ckflags(seL4_Word faulttype, seL4_Word addr, pageDirectory *pd) {
}

int new_region(pageDirectory * pd, seL4_Word start,
		size_t len, seL4_Word flags){
// A define a new region, that we can compare against when we try
// to declare a new frame

	region * reg = malloc(sizeof(region));
	if(!reg){
		return -1;
	}

	reg->vbase = start;
	reg->size = len;
	reg->flags = flags;
	region * head = pd->regions;
	if(!head){
		pd->regions = reg;
	}
	else{
		while(head->next){
			head = head->next;
		}
		head->next = reg;
	}
	return 0;
}


void PD_destroy(pageDirectory * pd){
	for(int i = 0; i < VM_PDIR_LENGTH; i++){
		if(pd->pTables[i]){
			PT_destroy(pd->pTables[i]);
			cspace_delete_cap(cur_cspace, pd->pTables_CPtr[i]);
			ut_free(pd->pTables_pAddr[i], seL4_PageTableBits);
		}
	}
	cspace_delete_cap(cur_cspace, pd->PageD);
	ut_free(pd->PD_addr, seL4_PageDirBits);
	free(pd);
	pd = NULL;
}

void PT_destroy(pageTable * pt){
	for(int i = 0; i < VM_PTABLE_LENGTH; i++){
		if(pt->frameIndex[i]){
			 frame_free(pt->frameIndex[i]);
		}
	}
}

region * find_region(pageDirectory *  pd, seL4_Word vAddr){
	if(pd){
		region * head = pd->regions;
		while(head){
			if((head->flags & REGION_STACK) == 0){// Regions grow up, Unless they are stacks.		
				if((vAddr >= head->vbase) && vAddr < (head->vbase + head->size)){
					return head;
				}
			}
			else{
				if(vAddr <= head->vbase && vAddr > (head->vbase + head->size)){
					return head;
				}
			}
			head = head->next;
		}
	}
	return NULL; //Either no regions are defined, or we couldn't find one that fits. So return NULL
}

void free_region_list(region * head){
	while(head){
		region * tmp = head;
		head = head->next;
		free(tmp);
	}
} 
