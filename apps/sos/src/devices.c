#include "devices.h"
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sos.h>
#include <sos/rpc.h>
#include "vm/addrspace.h"
#include "fdtable.h"

#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>
#include <sel4/sel4.h>
typedef int (*finish_read_t)(char* buffer, int len, seL4_CPtr reply, shared_region* shared_region);

char serialbuffer[IO_BUFFER_LENGTH];
int serialReadIndex = 0;
int serialWriteIndex = 0;
int newlineIndex = -1;
int buffLen = 0;

finish_read_t finish_func = NULL;
char* finish_buff = NULL;
int finish_len = NULL;
seL4_CPtr finish_reply = NULL;
shared_region* finish_region = NULL;

void setup_finish(char* buff, int len, seL4_CPtr reply, shared_region* shared_region, finish_read_t read_finish){
    finish_func = read_finish;
	finish_buff = buff;
	finish_len = len;
	finish_reply = reply;
    finish_region = shared_region;
}

int serial_read(struct serial* serial, char* buff, int len, seL4_CPtr reply, shared_region* shared_region){
    dprintf(0, "in devices, serial read\n");
	if(buffLen < len && newlineIndex == -1){
		setup_finish(buff, len, reply, shared_region, (read_t)read_finish);
	} else {
        read_finish(buff, len, reply, shared_region);
    }

	return 0;
}

int serial_write(struct serial* serial, char* buff, int len){
	return 0;
}

void read_finish(char* buff, int len, seL4_CPtr reply, shared_region *shared_region){
    dprintf(0, "in read finish \n");
	if(buffLen < len && newlineIndex == -1) {
        dprintf(0, "returning wihtout reply in read finish \n");
        return;
    }

	if(buffLen > len){
		if(serialReadIndex < serialWriteIndex){
			memcpy(buff, serialbuffer + serialReadIndex, len);
		}
		else{
            int buffer_left = IO_BUFFER_LENGTH - serialReadIndex;
            memcpy(buff, serialbuffer + serialReadIndex, buffer_left);
            memcpy(buff + buffer_left, serialbuffer, len - buffer_left); 
		}
        buffLen -= len;
        serialReadIndex += len;
        finish_func = NULL;
        return_reply(len);
	} else if(newlineIndex != -1){
        int offset = 0;
		if(newlineIndex > serialReadIndex){
            offset = newlineIndex - serialReadIndex;
			memcpy(buff, serialbuffer + serialReadIndex, offset);
		} else {
            int buffer_left = IO_BUFFER_LENGTH - serialReadIndex;
            offset = buffer_left + newlineIndex;
            memcpy(buff, serialbuffer + serialReadIndex, buffer_left);
            memcpy(buff + buffer_left, serialbuffer, offset - buffer_left); 
		}
        buffLen -= offset;
        finish_func = NULL;
        return_reply(offset);
    } 
    dprintf(0, "returning wihtout reply in read finish \n");
    return;
}

void return_reply(int ret) {
    dprintf(0, "returning from serial_red, ret is %d \n", ret);
    put_to_shared_region(finish_region, finish_buff);
    dprintf(0, "put to shared region passed \n", ret);
    seL4_MessageInfo_t reply = seL4_MessageInfo_new(0, 0, 0, 3);
    seL4_SetMR(0, ret);
    seL4_SetMR(1, finish_region->user_addr);
    seL4_Send(finish_reply, reply);
}

void serial_callback(struct serial * serial, char c){
    dprintf(0, "in serial_callback, got char %c\n", c);
	if(serialWriteIndex + 1 != serialReadIndex){
		serialbuffer[serialWriteIndex] = c;
		if(c == '\n') {
            newlineIndex = serialWriteIndex;
        }
		serialWriteIndex++;
		buffLen++;
		if(serialWriteIndex == IO_BUFFER_LENGTH){
			serialWriteIndex = 0;	
		}
	}
	if(finish_func){
		finish_func(finish_buff, finish_len, finish_reply, finish_region);
	}
}
