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
fdnode  *swapfile;
fhandle_t *mnt_point;

int pf_init(fhandle_t *mnt_point, fdnode *fd){
    fd->file = 1;
    fd->type = fdFile;
    fd->permissions = fdReadWrite;
    swapfile = fd;
    mnt_point = mnt_point;

	nfs_lookup(mnt_point, PAGEFILE_NAME, pf_open_complete, (uintptr_t)0);

    //TODO fix this, maybe call reply on an endpoint?
    while(!init) {}
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
		nfs_create(mnt_point, PAGEFILE_NAME, &attr, pf_open_create_complete, token);
		return;
	} else if(status == NFS_OK){
		int ok = 0;
		//Compare permissions, save to fdtable, go home.
        dprintf(0,"Permissions okay\n");
        swapfile->file = (seL4_Word)malloc(sizeof(fhandle_t));
        memcpy((void*)swapfile->file, fh, sizeof(fhandle_t));
	} else {
		dprintf(0, "Something happened\n");
	}

    init = 1;
}

void pf_open_create_complete(uintptr_t token, nfs_stat_t status, fhandle_t * fh, fattr_t * fattr){
	fdnode* fd = swapfile;
	if(status == NFS_OK){
			fd->file = (seL4_Word)malloc(sizeof(fhandle_t));
			memcpy((void*)fd->file, fh, sizeof(fhandle_t));
	} else {
		dprintf(0, "Something happened\n");
	}

    init = 1;
}

int pf_flush_entry(){
}

