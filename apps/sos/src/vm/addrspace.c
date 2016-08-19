#include "addrspace.h"

pageDirectory* pageTable_create(void){
	//Create a new Page Directory
	pageDirectory* pd = malloc(sizeof pageDirectory);
	pd->PD_addr = ut_alloc(seL4_PageDirBits);
	int err = cspace_ut_retype_addr(pd->PD_addr, seL4_ARM_PageDirectoryObject,
		seL4_PageDirBits, cur_cspace, &(pd->PageD));

	if(err){
		dprintf(0,"Could not retype memory in pageTable_create!\n");
	}
	
	regions = NULL;

	return pd;
}

int new_region(pageDirectory pd, seL4_Word start,
		size_t len, seL4_Word flags){
// A define a new region, that we can compare against when we try
// to declare a new frame

	region * reg = malloc(sizeof region);
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


void PD_destory(pageDirectory * pd){
	for(int i = 0; i < VM_PDIR_LENGTH; i++){
		if(pd->pTables[i]){
			PT_destory(pd->pTables[i]);
			cspace_delete_cap(cur_cspace, pd->pTables_CPtr[i]);
			ut_free(pd->pTables_pAddr[i]);
		}
	cspace_delete_cap(cur_cspace, PageD);
	ut_free(PD_addr);
	free(pd);
	pd = NULL;
}

void PT_destory(pageTable * pt){
	for(int i = 0; i < VM_PTABLE_LENGTH; i++){
		if(pt->frameIndex[i]) frame_free(pt->frameIndex[i]);
	}
}

struct region* find_region(pageDirectory pd, seL4_Word vAddr){


}

void free_region_list(struct region* head){
	while(head){
		region * tmp = head;
		head = head->next;
		free(tmp);
	}
} 
