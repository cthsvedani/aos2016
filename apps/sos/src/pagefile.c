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

extern fdnode  swapfile;
extern fhandle_t mnt_point;
unsigned int __pf_init = 0;

int pf_init(){
    swapfile.file = 1;
    swapfile.type = fdFile;
    swapfile.permissions = fdReadWrite;

	nfs_lookup(&mnt_point, "pagefile", pf_open_complete, 0);

    return 1;
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
        swapfile.file = (seL4_Word)malloc(sizeof(fhandle_t));
		if(!swapfile.file){
			panic("Failed to Initialize Pagefile\n");
		}
        memcpy((void*)swapfile.file, fh, sizeof(fhandle_t));
	} else {
		panic("Failed to initialize pagefile\n");
	}

    __pf_init = 1;
}

void pf_open_create_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	fdnode* fd = &swapfile;
	if(status == NFS_OK){
			fd->file = (seL4_Word)malloc(sizeof(fhandle_t));
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

