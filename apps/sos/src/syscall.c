#include "cspace/cspace.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

#include "syscall.h"
#include "stdlib.h"
#include "timer.h"
#include "fsystem.h"

static void return_reply(seL4_CPtr reply_cap, int ret) {
    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 2);
    seL4_SetMR(0, ret);
    seL4_Send(reply_cap, reply);
}

int sos_sleep(int msec, seL4_CPtr reply_cap){
	seL4_CPtr* data = malloc(sizeof(seL4_CPtr));
	if(data == NULL){
		dprintf(0,"Failed malloc in sos_sleep\n");
		return -1;
	}

	*data = reply_cap;
	register_timer(msec * 1000, (timer_callback_t)sos_wake, (void*)data);

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
	} else {
    return -1;
    }
	//We don't have a filesystem yet, so we only pass the string to the device list.
}

int sos_close(fdnode* fdtable, int index){
	if(index < 0 || index > MAX_FILES) return 0; //The index doesn't make sense, ignore it.
	if(fdtable[index].file != 0){
		close_device(fdtable, index); //We don't have a file system, Only care about devices.
	}
	return 0;
}

int handle_sos_read(seL4_CPtr reply_cap, pageDirectory * pd, fdnode* fdtable){
    dprintf(0, "in sos_sys_read\n");
    seL4_Word user_addr = seL4_GetMR(2);
    size_t count = seL4_GetMR(3);
    shared_region *shared_region = get_shared_region(user_addr, count,
                                            pd, fdWriteOnly);
    int file = seL4_GetMR(1);

    if(file > 0 && file <= MAX_FILES){
        fdnode *f_ptr = &fdtable[file];
        if(!f_ptr && !(f_ptr->permissions == fdReadOnly || f_ptr->permissions == fdReadWrite)) {
            dprintf(0, "file not found or wrong permissions \n");
            free_shared_region_list(shared_region);
            return_reply(reply_cap, -1);
            return 0;
        }
        if(f_ptr->type == fdDev) {
            char *buf = malloc(sizeof(char) * count);
            if(buf == NULL){
                panic("Buf allocation failed in read\n");
            }
            fdDevice* dev = (fdDevice*)fdtable[file].file;
            get_shared_buffer(shared_region, count, buf);
            dev->read(dev->device, buf, count, reply_cap, shared_region);
        } else if(f_ptr->type == fdFile) {
            fs_read((fhandle_t*)f_ptr->file, shared_region, reply_cap, count, f_ptr->offset);
        } else {
            free_shared_region_list(shared_region);
            return_reply(reply_cap, -1);
            return 0;
        }
    }

    return 1;
}

int handle_sos_write(seL4_CPtr reply_cap, pageDirectory * pd, fdnode* fdtable){
            seL4_Word user_addr = seL4_GetMR(2);
            size_t count = seL4_GetMR(3);

            shared_region *shared_region = get_shared_region(user_addr, count, 
                                                    pd, fdReadOnly);
			if(shared_region == NULL){
				seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 2);
				seL4_SetMR(0, 0);
				seL4_Send(reply_cap, reply);
			return 0;		
	}
            char *buf = malloc(sizeof(char) * count);
			if(buf == NULL){
				dprintf(0,"Malloc failed in sys_write\n");
			}
            get_shared_buffer(shared_region, count, buf);
            /*dprintf(0, "in syscall1: user v_addr is 0x%x size is %d\n",*/
                   /*seL4_GetMR(1), count); */

			int ret = -1;
			int file = seL4_GetMR(1);
			if(file > 0 && file <= MAX_FILES){
				fdDevice* dev = (fdDevice*)fdtable[file].file;
		 	    if(fdtable[file].file != 0 && 
						(fdtable[file].permissions == fdWriteOnly || fdtable[file].permissions == fdReadWrite)){
					ret = dev->write(dev->device, buf, count);
				} 
			}
			else if(file == 0){
				out(outDev, buf, count);
			}

            //need to free whole mem
            free(buf);
			buf = NULL;

            /*dprintf(0, "ret is %d\n", ret);*/
            seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 2);
            seL4_SetMR(0, ret);
            seL4_Send(reply_cap, reply);
			free_shared_region_list(shared_region);
			return 0;
}


int handle_sos_open(seL4_CPtr reply_cap, pageDirectory * pd, fdnode* fdtable){
			seL4_Word user_addr = seL4_GetMR(1);
			size_t count = seL4_GetMR(2);
			shared_region * shared_region = get_shared_region(user_addr, count, pd, fdReadOnly);
			char *buf = malloc(count*sizeof(char));
			if(buf == NULL){
				dprintf(0,"Malloc failed in sys_open\n");
			}
			get_shared_buffer(shared_region, count, buf);
			dprintf(0,"Attempting Open\n");
			int ret = sos_open(buf, fdtable, seL4_GetMR(3));
			seL4_MessageInfo_t reply = seL4_MessageInfo_new(0,0,0,1);
			seL4_SetMR(0, ret);
			seL4_Send(reply_cap, reply);
			free(buf);
			buf = NULL;
			return 0;
}

int handle_sos_stat(seL4_CPtr reply_cap, pageDirectory * pd){
		seL4_Word user_addr = seL4_GetMR(1);
		size_t count = seL4_GetMR(2);
		seL4_Word user_stat = seL4_GetMR(3);
		size_t size = count;
		if(size < sizeof(stat_t)){
			size = sizeof(stat_t);
		}
		shared_region * shared_region = get_shared_region(user_addr, count+1, pd, fdReadOnly);
		char *buf = malloc(size*sizeof(char));//We need to make sure the buffer is big enough to take the return as well!
		if(buf == NULL){
			dprintf(0,"Malloc failed in sos_stat\n");
		}
		get_shared_buffer(shared_region, count+1, buf);
        dprintf(0, "in sos_stat %s count is %d \n", buf,count);
		//After extracting the string, we don't need the shared_region for the name anymore, we can store
		//the shared region for the stat block instead.
		free_shared_region_list(shared_region);	
		shared_region = get_shared_region(user_stat, size, pd, fdReadOnly);	
		fs_stat(buf, shared_region, reply_cap);
		return 1;
} 

int handle_sos_getdirent(seL4_CPtr reply_cap, pageDirectory * pd){
		seL4_Word user_addr = seL4_GetMR(1);
		size_t count = seL4_GetMR(2);
		int pos = seL4_GetMR(3);
		shared_region * shared_region = get_shared_region(user_addr, count, pd, fdReadOnly);
        char *buf = malloc(count*sizeof(char));
		fs_getDirEnt(buf, shared_region, reply_cap, pos, count);
		return 1;
}
