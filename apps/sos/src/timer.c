#include <clock/clock.h>
#include <cspace/cspace.h>
#include "platsupport/plat/epit_constants.h"
#include <assert.h>

#include <clock/epit.h>
#include "mapping.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <autoconf.h>
#define verbose 5
#include <sys/debug.h>
#include <sys/panic.h>

/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *    interrupt_ep:       A (possibly badged) async endpoint that the driver
                          should use for deliverying interrupts to
 *
 * Returns CLOCK_R_OK iff successful.
 */
seL4_IRQHandler _irq_cap;
static seL4_CPtr _irq_ep;
EPIT epit[2];
EPIT *epit_1;


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
    dprintf(0, "got IRQ");

	/*	Ack IRQ */
    epit_1->REG_Status = 1;
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
    dprintf(0, "enable IRQ\n");

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

    epit_1 = (EPIT *) vstart;
    epit_init(epit_1);
    dprintf(0, "EPIT1_CR %d \n", epit_1->REG_Control);
    dprintf(0, "EPIT1_SR %d \n", epit_1->REG_Status);
    dprintf(0, "EPIT1_LR %d \n", epit_1->REG_Load);
    dprintf(0, "EPIT1_CMPR %d \n", epit_1->REG_Compare);
    dprintf(0, "EPIT1_CNR %d \n", epit_1->REG_Counter);

    dprintf(0, "EPIT1_CR address %x \n", &(epit_1->REG_Control));

    /*epit_setTime(*epit_1, 10, 1);*/
    /*epit_startTimer(*epit_1);*/
    dprintf(0, "EPIT1_CNR %d \n", epit_1->REG_Counter);
}

/*
Functions to Manage hardware. Defines located in epit.h.

Init will clear Enabled, Input Overwrite and Output Mode.
It will enable Peripheral Clock, Enable mode 1 (LR or 0xFFFF_FFFF), interupts and Sets Reload mode

The Prescale is 3300, or 1 count every 0.00005 seconds.
*/

void epit_init(EPIT *timer){
    uint32_t *CR = &(timer->REG_Control);

    //1. Disable the EPIT - set EN=0 in CR
    //2. Disable EPIT output
    //3. Disable EPIT interrupts
    *CR &= 0;

    //4. Program CLKSRC
    //Set clock source
    *CR |= EPIT_CLK_PERIPHERAL;
    *CR |= (EPIT_PRESCALE_CONST | EPIT_CLK_PERIPHERAL | EPIT_RLD); 

    //5. Clear the EPIT status register
    timer->REG_Status = 1; //Clear Status Register

    //6. Enable EPIT
    //EPIT enabled
    *CR |= (EPIT_EN_MOD);
    //set Load register
    timer->REG_Load = 100;

    //Activate in stop mode/ wait mode / debug mode
    /**CR |= (EPIT_STOP_EN | EPIT_WAIT_EN | EPIT_DB_EN);*/

    //Write to load register results in immediate overwriting of counter value
    *CR |= (EPIT_I_OVW);
    timer->REG_Load = 100;
    *CR &= (0xFFFF ^ (EPIT_I_OVW));

    //7.Enable EPIT
    //EPIT enabled
    *CR |= (EPIT_EN);
    //Compare Interrupt enabled
    *CR |= (EPIT_OCI_EN);

    timer->REG_Status = 1; //Clear Status Register

    //Disable the timer, Interrupt and Output of the Timer
    /**CR &= 0xFFFF | (EPIT_EN | EPIT_I_OVW | EPIT_OCI_EN | EPIT_OM | EPIT_PRESCALE_MSK | EPIT_CLK_MSK); */
    //Set Prescaler to defined prescale value, set to periph clock, and enable reloading
    /**CR |= (EPIT_PRESCALE_CONST | EPIT_CLK_PERIPHERAL | EPIT_RLD); */
    /*timer->REG_Status = 1; //Clear Status Register*/
    /**CR |= (EPIT_EN_MOD | EPIT_OCI_EN); //Timer will reset from 0xFFFF_FFFF or Load Value*/
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
