#include <clock/clock.h>
#include <cspace/cspace.h>
#include "platsupport/plat/epit_constants.h"
#include <assert.h>

#include <clock/epit.h>
#include "mapping.h"
#include "timer.h"

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


//Control Register Defines;

/*******************
 *** IRQ handler ***
 *******************/
void 
clock_irq(void) {
    int err;
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
    assert(!err);
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
    uintptr_t pstart = (uintptr_t)EPIT1_DEVICE_PADDR;
	seL4_Word vstart = map_device((void*)pstart, 40);

}

/*
Functions to Manage hardware. Defines located in epit.h.

Init will clear Enabled, Input Overwrite and Output Mode.
It will enable Peripheral Clock, Enable mode 1 (LR or 0xFFFF_FFFF), interupts and Sets Reload mode

The Prescale is 3300, or 1 count every 0.00005 seconds.
*/

void epit_init(EPIT timer){
	uint32_t *CR = &(timer.REG_Control);
    //Disable the timer, Interrupt and Output of the Timer
    *CR &= 0xFFFF | (EPIT_EN | EPIT_I_OVW | EPIT_OCI_EN | EPIT_OM | EPIT_PRESCALE_MSK | EPIT_CLK_MSK); 
    //Set Prescaler to defined prescale value, set to periph clock, and enable reloading
    *CR |= (EPIT_PRESCALE_CONST | EPIT_CLK_PERIPHERAL | EPIT_RLD); 
	timer.REG_Status = 1; //Clear Status Register
    *CR |= (EPIT_EN_MOD | EPIT_OCI_EN); //Timer will reset from 0xFFFF_FFFF or Load Value
	//Timer is ready to go.
}

void epit_setTime(EPIT timer, uint32_t milliseconds, int reset){
	//IF reset is non-zero We'll tell the timer to restart from our new value.
	if(reset){
		timer.REG_Control |= EPIT_I_OVW;	
	}

	uint32_t newCount = milliseconds * 20; //Prescaler is set for 0.05 of a millisecond.
	timer.REG_Load = newCount;

	if(reset){
        timer.REG_Control &= (0xFFFF ^(EPIT_I_OVW));
	}

}

void epit_startTimer(EPIT timer){
	timer.REG_Control |= EPIT_EN;
}

void stop_EPIT(EPIT timer){
	timer.REG_Control &= (0xFFFF ^ EPIT_EN);
}

int epit_timerRunning(EPIT timer){
	return (timer.REG_Control & EPIT_EN);
}

uint32_t epit_getCount(EPIT timer){
	return (timer.REG_Counter);
}

uint32_t epit_currentCompare(EPIT timer){
	return (timer.REG_Compare);
}
