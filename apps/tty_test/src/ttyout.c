/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/****************************************************************************
 *
 *      $Id:  $
 *
 *      Description: Simple milestone 0 code.
 *      		     Libc will need sos_write & sos_read implemented.
 *
 *      Author:      Ben Leslie
 *
 ****************************************************************************/

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ttyout.h"

#include <sel4/sel4.h>

void ttyout_init(void) {
    /* Perform any initialisation you require here */
}

static size_t sos_debug_print(const void *vData, size_t count) {
    size_t i;
    const char *realdata = vData;
    for (i = 0; i < count; i++)
        seL4_DebugPutChar(realdata[i]);
    return count;
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

size_t sos_read(void *vData, size_t count) {
    //implement this to use your syscall
    return 0;
}

