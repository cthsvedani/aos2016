/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sos.h>
#include <sos/rpc.h>

#include <sel4/sel4.h>

#define SYSCALL_ENDPOINT_SLOT    1

int sos_sys_open(const char *path, fmode_t mode) {
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,4);
	int count = strlen(path);
	seL4_SetMR(0, SOS_SYS_OPEN);
	seL4_SetMR(1, path);
	seL4_SetMR(2, count);
	seL4_SetMR(3, mode);
	seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
	return seL4_GetMR(0);
}

int sos_sys_read(int file, char *buf, size_t nbyte) {
    int ret;
    ret = sos_read(buf, nbyte);
    return ret;
}
size_t sos_read(void *vData, size_t count) {
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,3);
    seL4_SetMR(0, SOS_SYS_READ);
    rpc_call_data(tag, vData, count, SYSCALL_ENDPOINT_SLOT);
    return seL4_GetMR(0);
}

int sos_sys_write(int file, const char *buf, size_t nbyte){
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,4);
    seL4_SetMR(0, SOS_SYS_WRITE);
	seL4_SetMR(1, file);
	seL4_SetMR(2, buf);
	seL4_SetMR(3, nbyte);
	seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
    return seL4_GetMR(0);
}
size_t sos_write(void *vData, size_t count) {
	return 0;
}

void sos_sys_usleep(int msec) {
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,2);
	seL4_SetMR(0, SOS_SYS_SLEEP);
	seL4_SetMR(1, msec);
    rpc_call_mr(tag, SYSCALL_ENDPOINT_SLOT);
}

int64_t sos_sys_time_stamp(void) {
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	seL4_SetMR(0, SOS_SYS_TIMESTAMP);
    rpc_call_mr(tag, SYSCALL_ENDPOINT_SLOT);
	int64_t time = seL4_GetMR(0);
	time = time << 32;
	time += seL4_GetMR(1);
	return time;
}


int sos_sys_close(int file){
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,2);
	seL4_SetMR(0, SOS_SYS_CLOSE);
	seL4_SetMR(1, file);
	seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
	return 0;
}
