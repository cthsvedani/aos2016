#include "cspace/cspace.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

#include "syscall.h"
#include "stdlib.h"
#include "timer.h"

int sos_sleep(int msec, seL4_CPtr reply_cap){
	seL4_CPtr* data = malloc(sizeof(seL4_CPtr));
	if(data == NULL){
		dprintf(0,"Failed malloc in sos_sleep\n");
		return -1;
	}

	*data = reply_cap;
	uint32_t timer = register_timer(msec * 1000, (timer_callback_t)sos_wake, (void*)data);

	return 0;
}

uint32_t sos_brk(long newbreak, pageDirectory * pd, region * heap){
	if(newbreak == 0){
		dprintf(0, "PD is 0x%x, Heap is 0x%x, Break is 0x%x\n", pd, heap, heap->vbase + heap->size);
		return heap->vbase;
	}
	if(newbreak < heap->vbase || find_region(pd, newbreak)){
		return 0;
	}
	
	heap->size = newbreak - heap->vbase;
	dprintf(0, "PD is 0x%x, Heap is 0x%x, Break is 0x%x\n", pd, heap, heap->vbase + heap->size);
	return newbreak;
}

void sos_wake(uint32_t* id, void* data){
	seL4_CPtr reply = *((seL4_CPtr*)data);
	free(data);
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	seL4_SetMR(0, 0);
	seL4_Send(reply, tag);
	cspace_free_slot(cur_cspace, reply);
}

int sos_open(char* name, fdnode* fdtable, fd_mode mode){
	int fd = open_device(name, fdtable, mode);
	dprintf(0, "fd found is %d\n", fd);
	if(fd){
		return fd;
	}
	else return -1;
	//We don't have a filesystem yet, so we only pass the string to the device list.
}

void sos_close(fdnode* fdtable, int index){
	if(index < 0 || index > MAX_FILES) return; //The index doesn't make sense, ignore it.
	if(fdtable[index].file != 0){
		close_device(fdtable, index); //We don't have a file system, Only care about devices.
	}
}
