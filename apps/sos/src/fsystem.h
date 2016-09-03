#ifndef __SOS_FSYSTEM_H_
#define __SOS_FSYSTEM_H_

#include "sel4/types.h"
#include "fdtable.h"
#include "vm/addrspace.h"
#include "clock/clock.h"

#define NFS_TIME 100000

#define SERVER "192.168.168.1"

typedef void (*nfs_callback_t) (int request);

typedef struct request{
	nfs_callback_t finish;	
	seL4_CPtr reply;
	char* kbuff;
	fdnode* fdtable;
	int index;
	shared_region* s_region;
}nfs_request;

void fsystemStart();
void timeout(int* id, void* data);

#endif
