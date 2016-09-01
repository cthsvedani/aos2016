#include "devices.h"
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sos.h>
#include <sos/rpc.h>
#include "vm/addrspace.h"
#include "fdtable.h"

#define verbose 0
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
	dprintf(0, "Seting up reply function\n");
    finish_func = read_finish;
	finish_buff = buff;
	finish_len = len;
	finish_reply = reply;
    finish_region = shared_region;
}

int serial_read(struct serial* serial, char* buff, int len, seL4_CPtr reply, shared_region* shared_region){
	if(buffLen < len && newlineIndex == -1){
		setup_finish(buff, len, reply, shared_region, (read_t)read_finish);
	} else {
        read_finish(buff, len, reply, shared_region);
    }

	return 0;
}

int serial_write(struct serial* serial, char* buff, int len){
    int ret;
    ret = serial_send(serial, buff, len);
	return ret;
}

void read_finish(char* buff, int len, seL4_CPtr reply, shared_region *shared_region){
    dprintf(0, "in read finish \n");
	if(buffLen < len && newlineIndex == -1) {
        dprintf(0, "returning wihtout reply in read finish case 1\n");
        return;
    }

	if(buffLen >= len){
		if(serialReadIndex < serialWriteIndex){
			memcpy(buff, serialbuffer + serialReadIndex, len);
        }else{
            int buffer_left = IO_BUFFER_LENGTH - serialReadIndex;
            memcpy(buff, serialbuffer + serialReadIndex, buffer_left);
            memcpy(buff + buffer_left, serialbuffer, len - buffer_left); 
		}
        buffLen -= len;
        serialReadIndex += len;
        serialReadIndex = serialReadIndex % IO_BUFFER_LENGTH;
        finish_func = NULL;
        dprintf(0, "calling return reply with len %d\n", len);
        return_reply(len);
		return;
	} else if(newlineIndex != -1){
        int offset = 0;
        //If they are equal len will 1
		if(newlineIndex >= serialReadIndex){
            //Make sure you use paranthesis when you do aritmetic!
            offset = (newlineIndex - serialReadIndex) + 1;
			memcpy(buff, serialbuffer + serialReadIndex, offset);
            dprintf(0, "calculating offset newlineindex > serial index\n");
            dprintf(0, "newlineindex %d, serialreadindex %d, offset %d\n", newlineIndex, serialReadIndex, offset);
		} else {
            int buffer_left = IO_BUFFER_LENGTH - serialReadIndex;
            offset = buffer_left + newlineIndex;
            memcpy(buff, serialbuffer + serialReadIndex, buffer_left);
            memcpy(buff + buffer_left, serialbuffer, offset - buffer_left + 1); 
            dprintf(0, "calculating offset neslineindex !> serial index\n");
		}
        buffLen -= offset;
        serialReadIndex += offset;
        serialReadIndex = serialReadIndex % IO_BUFFER_LENGTH;
		newlineIndex = -1;
        finish_func = NULL;
        dprintf(0, "calling return reply bufflen < len with len %d\n", offset);
        return_reply(offset);
		return;
    } 
    dprintf(0, "returning without reply in read finish nonsense case\n");
    return;
}

void return_reply(int ret) {
    dprintf(0, "replying with %d, containing finish_buff %s", ret, finish_buff);
    put_to_shared_region(finish_region, finish_buff);
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
			dprintf(0, "OH NO A NEWLINE WHAT DO?!\n");
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
