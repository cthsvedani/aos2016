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

unsigned volatile int init = 0;
extern fdnode  swapfile;
extern fhandle_t mnt_point;

int pf_init(){
    swapfile.file = 1;
    swapfile.type = fdFile;
    swapfile.permissions = fdReadWrite;

	uint32_t* index = malloc(sizeof(uint32_t));
	nfs_lookup(&mnt_point, "pagefile", pf_open_complete, (uintptr_t)index);

    //TODO fix this, maybe call reply on an endpoint?
    /*while(!init) {}*/
    return 1;
}

int pf_open_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
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
	} else if(status == NFS_OK){
        swapfile.file = (seL4_Word)malloc(sizeof(fhandle_t));
        memcpy((void*)swapfile.file, fh, sizeof(fhandle_t));
	} else {
		dprintf(0, "Something happened in open complete, status is %d\n", status);
	}

    init = 1;
}

void pf_open_create_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	fdnode* fd = &swapfile;
	if(status == NFS_OK){
			fd->file = (seL4_Word)malloc(sizeof(fhandle_t));
			memcpy((void*)fd->file, fh, sizeof(fhandle_t));
	} else {
		dprintf(0, "Something happened in open create complete\n");
	}

    init = 1;
}

int pf_flush_entry(){
}

