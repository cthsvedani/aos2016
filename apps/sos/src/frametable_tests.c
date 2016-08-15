#include <cspace/cspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "frametable.h"
#include "frametable_tests.h"
#include <assert.h>
#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

void ftest(){
    int page;
	/* Allocate 10 pages and make sure you can touch them all */
	for (int i = 0; i < 10; i++) {
	    /* Allocate a page */

	    seL4_Word ret;
        frame_alloc(&ret);
	    assert(ret);
		uint32_t * vaddr = (uint32_t *) ret;
	    /* Test you can touch the page */
        *vaddr = 0x37;
	    assert(*vaddr == 0x37);

	    printf("Page #%lu allocated at %p\n",  i, (void *) vaddr);
	}

	/* Test that you never run out of memory if you always free frames. */
    for (int i = 0; i < 10000; i++) {
        //[> Allocate a page <]/
        seL4_Word ret;
        page = frame_alloc(&ret);
        assert(ret);
        uint32_t * vaddr = (uint32_t *) ret;

        //[> Test you can touch the page <]
        *vaddr = 0x37;
        assert(*vaddr == 0x37);

        //[> print every 1000 iterations <]
        if (i % 1000 == 0) {
            printf("Page #%d allocated at %p\n",  i, vaddr);
        }

         frame_free(page);
    }

	/* Test that you eventually run out of memory gracefully,
	   and doesn't crash */
    while (1) {
          /*Allocate a page */
        seL4_Word ret;
        frame_alloc(&ret);
        /*assert(ret);*/
        uint32_t * vaddr = (uint32_t *) ret;
        if (!vaddr) {
          printf("Out of memory!\n");
          break;
        }
        /*[>Test you can touch the page <]*/
        *vaddr = 0x37;
        assert(*vaddr == 0x37);
	}
}

void ftest_cap() {

    seL4_Word page[10];
	/* Allocate 10 pages */
	for (int i = 0; i < 10; i++) {
	    /* Allocate a page */

	    seL4_Word ret;
        frame_alloc(&page[i]);
	    assert(page[i]);
		uint32_t * vaddr = (uint32_t *) page[i];
	    /* Test you can touch the page */
        *vaddr = 0x37;
	    assert(*vaddr == 0x37);

	    printf("Page #%lu allocated at %p\n",  i, (void *) vaddr);
	}

    /* De-allocate the 10 pages */
    for (int i = 0; i < 10; i++) {
        frame_free(page[i]);
        assert(page[i]);
        uint32_t * vaddr = (uint32_t *) page[i];
        /* Should generate cap fault */
        *vaddr = 0x37;
        assert(*vaddr == 0x37);
    }

}
