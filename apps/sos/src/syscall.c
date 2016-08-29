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
	uint32_t timer = register_timer(msec, &sos_wake, (void*)data);
	if(!timer){
		dprintf(0,"Help I'm stuck in an operating system %d\n", timer);
		return 1;
	}

	return 0;
}

uint32_t sos_brk(long newbreak, pageDirectory * pd, region * heap){
	if(newbreak == 0){
		return heap->vbase;
	}

	if(newbreak < heap->vbase || find_region(pd, newbreak)){
		return 0;
	}
	
	heap->size = newbreak - heap->vbase;
	
	return newbreak;
}

void sos_wake(int* id, void* data){
	seL4_CPtr reply = *((seL4_CPtr*)data);
	free(data);
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	seL4_SetMR(0, 0);
	seL4_Send(reply, tag);
	cspace_free_slot(cur_cspace, reply);
}
