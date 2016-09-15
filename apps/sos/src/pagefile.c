#include "pagefile.h"
#include "vm/addrspace.h"
#include "fdtable.h"
#include "proc.h"
#include "syscall.h"
#include <nfs/nfs.h>
#include "pagefile.h"
#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

extern fdnode  *swapfile;
extern fhandle_t mnt_point;
unsigned int __pf_init = 0;
pf_region *pf_freelist;

int pf_init(){
    swapfile = malloc(sizeof(fdnode));
    swapfile->file = malloc(sizeof(fhandle_t));
    swapfile->type = fdFile;
    swapfile->permissions = fdReadWrite;
    pf_freelist = malloc(sizeof(pf_region));
    if(!pf_freelist) {
        panic("Nomem: pf_init");
    }
    pf_freelist->next = NULL;
    pf_freelist->offset = 0;
    pf_freelist->size = PAGEFILE_PAGES;

	nfs_lookup(&mnt_point, "pagefile", pf_open_complete, 0);

    return 1;
}

int pf_free(int offset){
    pf_region *new_region = malloc(sizeof(pf_region));
    if(!new_region) {
        panic("Failed to create freelist node");
    }
    new_region->next = NULL;
    new_region->offset = offset;
    new_region->size = 1;

    if(pf_freelist) {
        pf_freelist->next = new_region;
    } else {
        pf_freelist = new_region;
    }
}

int pf_get_next_offset(){
    int ret;

    if(!pf_freelist){
        panic("Pagefile is full, we need to kill something!");
        return -1;
    }
    
    ret = pf_freelist->offset;
    pf_freelist->offset += 1;
    pf_freelist->size -= 1;
    if(!pf_freelist->size){ 
        pf_region *tmp = pf_freelist;
        pf_freelist = pf_freelist->next;
        free(tmp);
    }

    return ret;
}

void pf_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	if(status == NFSERR_NOENT){
		sattr_t attr;
		attr.mode = (6 << 6) + 6;
		attr.uid = 0;
		attr.gid = 0;
		attr.size = 0;
		attr.atime.seconds = -1;
		attr.atime.useconds = -1;
		attr.mtime.seconds = -1;
		attr.mtime.useconds = -1;
		nfs_create(&mnt_point, "pagefile", &attr, pf_open_create_complete, token);
		return;
	}

	if(status == NFS_OK){
        memcpy((void*)swapfile->file, fh, sizeof(fhandle_t));
	} else {
		panic("Failed to initialize pagefile\n");
	}

    __pf_init = 1;
}

void pf_open_create_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	fdnode* fd = swapfile;
	if(status == NFS_OK){
			memcpy((void*)fd->file, fh, sizeof(fhandle_t));
	} else {
		panic("Pagefile failed to init\n");
	}

    __pf_init = 1;
}

int pf_write_out(){
	return 0;
}

void pf_fault_in(){

}

void clock(){

}

