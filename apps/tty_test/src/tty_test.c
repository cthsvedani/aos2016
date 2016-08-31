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
 *      Description: Simple milestone 0 test.
 *
 *      Author:			Godfrey van der Linden
 *      Original Author:	Ben Leslie
 *
 ****************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sel4/sel4.h>
#include "sos.h"

#include "ttyout.h"

// Block a thread forever
// we do this by making an unimplemented system call.
static void
thread_block(void){
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetTag(tag);
    seL4_SetMR(0, 213);
    seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
}

int malloc_hammer(){
	int* bob;
	int i = 0;

	printf("\nStarting New Test\n");
	while(1){
		bob = malloc(1024*sizeof(int));
		if(!(i % 1000)){
			printf("At i = %d, Bob = %x\n", i, bob);
		}
		if(!bob){
			break;
		}
		bob[3] = 25;
		i++;
	}
	printf("Out Of Memory\n");
return 0;
}

#define NPAGES 27
#define TEST_ADDRESS 0x20000000
#define PAGE_SIZE_4K 4096

/* called from pt_test */
static void
do_pt_test(char *buf)
{
    int i;

    /* set */
    for (int i = 0; i < NPAGES; i++) {
	    buf[i * PAGE_SIZE_4K] = i;
    }

    /* check */
    for (int i = 0; i < NPAGES; i++) {
	    assert(buf[i * PAGE_SIZE_4K] == i);
    }
}

static void
pt_test( void )
{
    /* need a decent sized stack */
    char buf1[NPAGES * PAGE_SIZE_4K], *buf2 = NULL;

    /* check the stack is above phys mem */
    assert((void *) buf1 > (void *) TEST_ADDRESS);

    /* stack test */
    do_pt_test(buf1);
	printf("Stack Okay\n");
    /* heap test */
    buf2 = malloc(NPAGES * PAGE_SIZE_4K);
    assert(buf2);
    do_pt_test(buf2);
	printf("Heap Okay\n");
    free(buf2);
}

int main(void){

    do {
		int f = sos_sys_open("console", FM_READWRITE);
		printf("Hello World!\n");
		char buff[2048];
		sos_sys_read(f, buff, 2048);
		printf("TTY got %s\n", buff); 
        thread_block();
    } while(1);

    return 0;
}
