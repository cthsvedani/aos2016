#include "pagefile.h"
#include "vm/addrspace.h"
#include "fdtable.h"
#include "proc.h"
#include "syscall.h"
#include <nfs/nfs.h>
#include "pagefile.h"
#include "setjmp.h"
#include "fsystem.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

extern fdnode  swapfile;
extern fhandle_t mnt_point;
unsigned int __pf_init = 0;
int hand;

int pf_init(){
    swapfile.file = 1;
    swapfile.type = fdFile;
    swapfile.permissions = fdReadWrite;
	hand = frameBot;
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
		dprintf(0, "???..?! %d\n", sizeof(fhandle_t));
		if(!swapfile.file){
			panic("Failed to Initialize Pagefile\n");
		}
        memcpy(bob, fh, sizeof(fhandle_t));
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

extern seL4_CPtr _sos_ipc_ep_cap;
extern void syscall_loop(seL4_CPtr ep);

void pf_write_out(int pfIndex, frame* fr){
	uint32_t offset = fr->index;
	shared_region * reg = malloc(sizeof(shared_region));
	reg->user_addr = 0;
	reg->vbase = VMEM_START + (offset << seL4_PageBits); 
	reg->size = 4096;
	reg->next = NULL;
	if(fr->pte != NULL) fr->pte->index = (frameTop + pfIndex);
	fs_write(&swapfile, reg, 4096, 0, (pfIndex << seL4_PageBits));
	syscall_loop(_sos_ipc_ep_cap);	
	panic("Err..?");
}
extern jmp_buf targ;

void pf_return(){
	longjmp(targ ,1);
}
void pf_fault_in(){

}
frame* clock(int force){
	if(force){
		int i = hand++;
		if(hand > frameTop) hand = frameBot;
		return &(ftable[i]);
	}
}

