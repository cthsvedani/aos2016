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


#include "ttyout.h"

// Block a thread forever
// we do this by making an unimplemented system call.
static void
thread_block(void){
    seL4_MessageInfo_t tag = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetTag(tag);
    seL4_SetMR(0, 2);
    seL4_Call(SYSCALL_ENDPOINT_SLOT, tag);
}

int malloc_hammer(){
	int* bob;
	int i = 0;
	while(1){
		bob = malloc(1024*sizeof(int));
		if((!(i%10)) || bob == 0x101e8010){
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


int main(void){
    /* initialise communication */
    ttyout_init();

	malloc_hammer();
	
    do {
        printf("task:\tHello world, I'm\ttty_test!\n");
        thread_block();
        // sleep(1);	// Implement this as a syscall
    } while(1);

    return 0;
}
