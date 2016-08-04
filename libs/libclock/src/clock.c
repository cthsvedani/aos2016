#include "clock/clock.h"
#include <cspace/cspace.h>
#include "platsupport/plat/epit_constants.h"
#include <assert.h>

/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *    interrupt_ep:       A (possibly badged) async endpoint that the driver
                          should use for deliverying interrupts to
 *
 * Returns CLOCK_R_OK iff successful.
 */
int irq;
seL4_IRQHandler _irq_cap;
static seL4_CPtr _irq_ep;

/*******************
 *** IRQ handler ***
 *******************/
void 
clock_irq(void) {
    int err;
    int i;
    /* skip if the network was not initialised */
    if(_irq_ep == seL4_CapNull){
        return;
    }

	/*	Handle IRQ */

	/*	Ack IRQ */
    err = seL4_IRQHandler_Ack(_irq_cap);
    assert(!err);
}

static seL4_CPtr
enable_irq(int irq, seL4_CPtr aep) {
    seL4_CPtr cap;
    int err;
    /* Create an IRQ handler */
    cap = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, irq);

    /* Assign to an end point */
    err = seL4_IRQHandler_SetEndpoint(cap, aep);

    /* Ack the handler before continuing */
    err = seL4_IRQHandler_Ack(cap);

    return cap;
}

int start_timer(seL4_CPtr interrupt_ep){
	//badged copy of the asyncEP
	_irq_ep = interrupt_ep;

	//the hardware interrupt line number
	irq = EPIT1_INTERRUPT;

	//create interrupt handling cap
	_irq_cap = enable_irq(irq, _irq_ep);

	//Map the device into the drivers virtual address space
	seL4_Word vstart = map_device(EPIT1_DEVICE_PADDR, 40);

}
