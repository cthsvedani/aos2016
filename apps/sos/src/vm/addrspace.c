#include "addrspace.h"

#include "ut_manager/ut.h"
#include "frametable.h"
#include "mapping.h"
#include "vmem_layout.h"
#include "fdtable.h"

#define verbose 0 
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
			dprintf(0,"Page fault\n");
            int ret = page_fault(pd, addr); 
            if(!ret) {
                return -2;			
            }
		}
		int err = sos_map_page(pd, frame, addr, seL4_AllRights, seL4_ARM_Default_VMAttributes);
		if(err){
			dprintf(0,"Page mapping at %x failed with error code %d\n",addr, err);
			return -3;
		}
	}

    return 0;
}

int page_fault(pageDirectory * pd, seL4_Word addr) {
    //chose entry in frametable to swap
    pf_flush_entry();

    //invalidate the page in PT
    //initialize page_flush
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
		dprintf(0,"Region Head is 0x%x\n", reg);
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
		if(pt->frameIndex[i].index){
			 frame_free(pt->frameIndex[i].index);
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

void free_shared_region_list(shared_region * head){
	while(head){
		shared_region * tmp = head;
		head = head->next;
		free(tmp);
	}
} 
void get_shared_buffer(shared_region *shared_region, size_t count, char *buf) {
    int i = 0;
    int buffer_index = 0;
    dprintf(0, "in get_shared_buffer with size %d\n", count);
    while(shared_region) {
        dprintf(0, "shared_region%d addr 0x%x, size %d \n", i, shared_region->vbase, shared_region->size);
        dprintf(0, "shared_region%d uaddr 0x%x, size %d \n", i, shared_region->user_addr, shared_region->size);
		dprintf(0, "kbuf = 0x%x\n", buf);
        memcpy(buf + buffer_index, (void *)shared_region->vbase, shared_region->size);
        buffer_index += shared_region->size;
        shared_region = shared_region->next; 
        i++;
    }

    dprintf(0, "exiting get_shared_buffer \n");
}


//the regions struct is only maintained by kernel is thus trusted.
void put_to_shared_region(shared_region *shared_region, char *buf) {
    uint32_t buffer_index = 0;
    dprintf(0, "put_to_shared entered \n");
    while(shared_region) {
        dprintf(0, "region vbase is %x, with size %d\n", shared_region->vbase, shared_region->size);
        dprintf(0, "kernel buf is %x\n", buf);
        memcpy((void *)shared_region->vbase, buf + buffer_index, shared_region->size);
        buffer_index += shared_region->size;
        shared_region = shared_region->next;
    }
    dprintf(0, "put_to_shared exiting \n");
}

void put_to_shared_region_n(shared_region **s_region, char *buf, size_t n) {
    uint32_t buffer_index = 0;
    dprintf(0, "put_to_shared_n entered \n");
    while(*s_region && (n > 0)) {
        dprintf(0, "region vbase is %x, with size %d\n", (*s_region)->vbase, (*s_region)->size);
        dprintf(0, "kernel buf is %x\n", buf);
        if(n >= (*s_region)->size) {
            memcpy((void *)((*s_region)->vbase), buf + buffer_index, (*s_region)->size);
            n -= (*s_region)->size;
			buffer_index += (*s_region)->size;
            shared_region* tmp = (*s_region);
            *s_region = (*s_region)->next; 
            free(tmp);
        }
		 else {
			dprintf(0, "Ending with n %d\n", n); 
            memcpy((void *)((*s_region)->vbase), buf + buffer_index, n);
            (*s_region)->vbase += n;
			(*s_region)->size -= n;	
            return;
        }

    }
    dprintf(0, "put_to_shared exiting \n");
}

shared_region * get_shared_region(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd, fd_mode mode) {
    //Reuse region structs to define our regions of memory
    shared_region *head = malloc(sizeof(shared_region));
    dprintf(0, "in get_shared_region with len %d\n", len);
	if(head == NULL){
		dprintf(0,"Malloc failed\n");
		return NULL;
	}
    shared_region *tail = head;
    head->user_addr = user_vaddr;
	if(len == 0){
		free(head);
		return NULL;
	}
    int i = 0;
    while(len > 0) {
        //check defined user regions
        if(!pt_ckptr(user_vaddr, len, user_pd, mode)) {
            dprintf(0,"Invalid memory region\n");
            free_shared_region_list(head);
            return NULL;
        }   

        //get the sos_vaddr that corresponds to the user_vaddr
        seL4_Word sos_vaddr = get_user_translation(user_vaddr, user_pd);
        if(!sos_vaddr) {
            dprintf(0, "Mapping does not exist.\n");
            free_shared_region_list(head);
            return NULL;
        }

        //this is the start address  this page
        tail->vbase = sos_vaddr + PAGE_OFFSET(user_vaddr);
		tail->user_addr = user_vaddr;
        //the length left on this page
        size_t page_len = PAGE_ALIGN(user_vaddr + (1 << seL4_PageBits)) - user_vaddr;

        //we need more regions
        if( len > page_len ) {
            tail->size = page_len;
            len -= page_len;
            tail->next = malloc(sizeof(region));
            //increment user_vaddr to find the next start of the page
            user_vaddr += tail->size;
            dprintf(0, "in shared region %d, len %d, vbase 0x%x, size %d \n", i, len, tail->vbase, tail->size);
            tail = tail->next;
        //no more regions
        } else {
            tail->size = len;
            len = 0;
            dprintf(0, "in shared region %d, len %d, vbase 0x%x, size %d \n", i, len, tail->vbase, tail->size);
        }
        tail->flags = seL4_AllRights;
        tail->next = NULL;
        i++;
    }
    
    return head;
}

//get the kernel_vaddr representation of user_vaddr
seL4_Word get_user_translation(seL4_Word user_vaddr, pageDirectory * user_pd) {
    uint32_t dindex = VADDR_TO_PDINDEX(user_vaddr);
    uint32_t tindex = VADDR_TO_PTINDEX(user_vaddr);
	//Check if the page is actually resident, if it isn't, fault it in.
	if(user_pd->pTables[dindex] == NULL || user_pd->pTables[dindex]->frameIndex[tindex].index == 0){
		vm_fault(user_pd, user_vaddr);
	}
    uint32_t index = user_pd->pTables[dindex]->frameIndex[tindex].index;
	dprintf(0,"Translated 0x%x to 0x%x\n", user_vaddr, VMEM_START + (ftable[index].index << seL4_PageBits));
    return VMEM_START + (ftable[index].index << seL4_PageBits);
}

int pt_ckptr(seL4_Word user_vaddr, size_t len, pageDirectory * user_pd, fd_mode mode) {
    region *start_region = find_region(user_pd, user_vaddr);
    region *end_region = find_region(user_pd, user_vaddr + len);

    if((start_region && end_region) && 
        (start_region->vbase == end_region->vbase)){
		if((mode == fdReadOnly && (start_region->flags & VM_FAULT_READ)) ||
				 (mode == fdWriteOnly && (start_region->flags & VM_FAULT_WRITE) )){
	        return 1;
		}
    }

    return 0;
}
