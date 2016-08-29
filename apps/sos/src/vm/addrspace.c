#include "addrspace.h"

#include "ut_manager/ut.h"
#include "frametable.h"
#include "mapping.h"
#include "vmem_layout.h"

#define verbose 5
#include "sys/debug.h"
#include "sys/panic.h"


pageDirectory* pageTable_create(void){
	//Create a new Page Directory
	pageDirectory* pd = malloc(sizeof(pageDirectory));
	conditional_panic(pd == NULL, "Couldn't allocate memory for the Page Directory!");
	
	pd->PD_addr = ut_alloc(seL4_PageDirBits);
	int err = cspace_ut_retype_addr(pd->PD_addr, seL4_ARM_PageDirectoryObject,
		seL4_PageDirBits, cur_cspace, &(pd->PageD));
	
	memset(pd->pTables, 0, VM_PDIR_LENGTH*sizeof(seL4_Word)); //seL4 zeros memory except when it doesn't

	if(err){
		dprintf(0,"Could not retype memory in pageTable_create!\n");
	}
	
	pd->regions = NULL;

	return pd;
}

int vm_fault(pageDirectory * pd, seL4_Word addr) {
	region * reg = find_region(pd, addr); 
	if(reg == NULL){
		return -1;
	}	
	else{
		int frame = frame_alloc();
		if(!frame){
			dprintf(0,"Memory Allocation Failed!\n");
			return -2;			
		}
		int err = sos_map_page(pd, frame, addr, seL4_AllRights, seL4_ARM_Default_VMAttributes);
		if(err){
			dprintf(0,"Page mapping at %x failed with error code %d\n",addr, err);
			return -3;
		}
	}

    return 0;
}

region * new_region(pageDirectory * pd, seL4_Word start,
		size_t len, seL4_Word flags){
// A define a new region, that we can compare against when we try
// to declare a new frame
    if(!start || find_region(pd, start) || find_region(pd,start + len)) {
        return NULL;
    }

    if(start > PROCESS_IPC_BUFFER || (start + len > PROCESS_IPC_BUFFER && !(
                    flags & REGION_STACK))){
        return NULL;
    }

	region * reg = malloc(sizeof(region));
	if(!reg){
		return NULL;
	}
	dprintf(0,"Adding New Region at %x , for %d bytes\n", start, len);
	reg->vbase = start;
	reg->size = len;
	reg->flags = flags;
	region * head = pd->regions;
	if(!head){
		pd->regions = reg;
	}
	else{
		while(head->next){           
            if((reg->vbase < head->vbase) && ((reg->vbase + len) > (head->vbase + head->size))) {
                return NULL;
            }
			head = head->next;
		}
		head->next = reg;
	}
	return reg;
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
			}else{
				if(vAddr <= head->vbase && vAddr > (head->vbase - head->size)){
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

region * get_shared_region(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd) {
    return 0;
}

seL4_Word get_user_translation(seL4_Word user_vaddr, pageDirectory * user_pd) {
    return 0;
}

int pt_ckptr(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd) {
    return 0;
}
