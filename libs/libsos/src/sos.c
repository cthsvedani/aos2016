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

#include <sel4/sel4.h>

#define SYSCALL_ENDPOINT_SLOT    1
#define SOS_SYS_SLEEP 126
#define SOS_SYS_TIMESTAMP 127

int sos_sys_open(const char *path, fmode_t mode) {
    assert(!"You need to implement this");
    return -1;
}

int sos_sys_read(int file, char *buf, size_t nbyte) {
    assert(!"You need to implement this");
    return -1;
}

size_t sos_write(void *vData, size_t count) {
	const char *realdata = vData;

	int left = count;
	int read = 0;
	while(left > 0 ) {
		int j;
		seL4_SetMR(0, 1);
		for(j = 1; j <= left && j <= seL4_MsgMaxLength; j++) {
			seL4_SetMR(j, realdata[read + (j - 1)]);
		}
		seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, j+1);
		seL4_SetTag(tag);
        seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
		int sent = (int)seL4_GetMR(0);
		read = read + sent;
		left = left - sent;
	}
	return read;
}

void sos_sys_usleep(int msec) {
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,2);
	seL4_SetMR(0, SOS_SYS_SLEEP);
	seL4_SetMR(1, msec);
	seL4_SetTag(tag);
	seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
}

int64_t sos_sys_time_stamp(void) {
	seL4_MessageInfo_t tag = seL4_MessageInfo_new(0,0,0,1);
	seL4_SetMR(0, SOS_SYS_TIMESTAMP);
	seL4_SetTag(tag);
	seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
	int64_t time = seL4_GetMR(0);
	time = time << 32;
	time += seL4_GetMR(1);
	return time;
}

int sos_sys_write(int file, const char *vData, size_t count) {
	const char *realdata = vData;

	int left = count;
	int read = 0;
	while(left > 0 ) {
		int j;
		seL4_SetMR(0, 1);
		for(j = 1; j <= left && j <= seL4_MsgMaxLength; j++) {
			seL4_SetMR(j, realdata[read + (j - 1)]);
		}
		seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, j+1);
		seL4_SetTag(tag);
        seL4_Call(1, tag);
		int sent = (int)seL4_GetMR(0);
		read = read + sent;
		left = left - sent;
	}
	return read;
}
