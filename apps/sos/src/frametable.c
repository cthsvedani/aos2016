#include "frametable.h"
#include "ut.h"

frame* ftable;
freeNode* freeList;
seL4_ARM_PageDirectory pd;


void frametable_init(seL4_Word low, seL4_Word high) { 
    seL4_Word size = high - low;

    seL4_Word count = size >> PAGE_BIT_SHIFT;
    ftable = malloc(count*sizeof(frame));
    assert(ftable);
    freeList_init(count); 
    //seL4_Word adr = ut_alloc(sizeof(frame));
    
    //set up a new page directory - page tables should be set up by map_page

    //malloc frame table

}

void freeList_init(seL4_Word count) { 


}

/*
 * The physical memory is reserved via the ut_alloc, the memory is retyped into a frame, and the frame is mapped into the SOS
 * window at a fixed offset of the physical address.
 */
void frame_alloc() {
    //ut_alloc
    
    //cspace_ut_retupe_addr to seL4_ARM_SmallPageObject
    
    //map_page
}

/*
 * The physical memory is no longer mapped in the window, the frame object is destroyed, and the physical memory ranged
 * is returned via ut_free
 */
void frame_free() {

}

//vspace is represented by a PD object
//PDs map page tables
//PTs map pages
//creating a PD (by retyping) creates the vspace
//deleting the PD deletes the vspace

//Code example:
//get untyped memory (a page size)
//seL4_Word frame_addr = ut_alloc(seL4_PageBits);

//retype memory to type seL4_ARM_Page
//cspace_ut_retype_addr(frame_addr, seL4_ARM_Page, seL4_ARM_Default_VMAttributes);

//insert mapping
//map_page(frame_cap, pd_cap, 0xA00000000, seL4_AllRights, seL4_ARM_Default_VMAttributes);

//zero the new memory
//bzero((void *)0xA0000000, PAGESIZE);

//each mapping requeires its own frame cap even for the same fram, and has:
//virtual_address, phys_address, address_space and frame_cap
//address_space struct identifies the level 1 page_directory cap
//you need to keep track of (frame_cap, PD_cap, v_adr, p_adr)

//
//sizes of memory objects
//
//OBJECT        Size in bytes (and alignment)
//Frame             2^12
//PageD             2^14
//PageT             2^10


