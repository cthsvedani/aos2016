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

//Number of timer overflows for the periodic timer
uint64_t timestamp_overflows;

//Our two timer registers
timer timers[2];

//The timer id allocation list
int freelist[CLOCK_MAX_TIMERS];

//The callback queue
callback_queue queue;

/*******************
 *** IRQ handler ***
 *******************/
void 
clock_irq(void) {
	/*	Handle IRQ */
    if(timers[0].reg->REG_Status) {
        handle_irq(timers[0]);
        handle_irq_callback();
    }
    if(timers[1].reg->REG_Status) {
        timestamp_overflows += 1;
        handle_irq(timers[1]);
    }
}

void
handle_irq_callback() {
    //timer overflow, need more time
    uint64_t timestamp = epit_getCurrentTimestamp();
    if(queue.head->timestamp > timestamp) {
        uint64_t delay = queue.head->timestamp - timestamp;
        epit_setTime(timers[0].reg, delay, 1);
    } else {

        //temp pointer
        callback_node_t *temp = queue.head;

        //change queue
        /*queue.head = queue.head->next;*/
        queue.head = NULL;

        /*Activate all timers that should have been fired already*/
        callback_node_t * cur = queue.head;
        while(cur && cur->timestamp <= timestamp){
            cur->callback(cur->id, cur->data);
            cur = cur->next;
        }
        if(cur){
            uint64_t delay = cur->timestamp - timestamp;
            epit_setTime(timers[0].reg, delay, 1);
            epit_startTimer(timers[0].reg);
        } else {
            epit_stopTimer(timers[0].reg);
        }

        //fire callback
        temp->callback(temp->id, temp->data);

        //free queue node
        free(temp);
     }        
}

void
handle_irq(timer timer) {
    int err;
    timer.reg->REG_Status = 1;
    err = seL4_IRQHandler_Ack(timer.cap);
    assert(!err);
}

static void
testfunc_print(int number, void* data) {
        dprintf(0, "In callback, number: %d, timestamp is %llu\n", number, epit_getCurrentTimestamp());
}


static seL4_CPtr
enable_irq(int irq, seL4_CPtr aep) {
    seL4_CPtr cap;
    int err;
    /* Create an IRQ handler */
    cap = cspace_irq_control_get_cap(cur_cspace, seL4_CapIRQControl, irq);
    assert(cap);
    /* Assign to an end point */
    err = seL4_IRQHandler_SetEndpoint(cap, aep);

    /* Ack the handler before continuing */
    err = seL4_IRQHandler_Ack(cap);
    assert(!err);
    return cap;
}

int start_timer(seL4_CPtr interrupt_ep){
	//badged copy of the asyncEP

     seL4_CPtr _irq_ep = interrupt_ep;

	//create interrupt handling cap
    timers[0].cap = enable_irq( EPIT1_INTERRUPT, _irq_ep);
    timers[1].cap = enable_irq( EPIT2_INTERRUPT, _irq_ep);

	//Map the device into the drivers virtual address space
    uintptr_t pstart_EPIT1 = (uintptr_t)EPIT1_DEVICE_PADDR;
    uintptr_t pstart_EPIT2 = (uintptr_t)EPIT2_DEVICE_PADDR;
    timers[0].reg = (EPIT *) map_device((void*)pstart_EPIT1, 20);
    timers[1].reg = (EPIT *) map_device((void*)pstart_EPIT2, 20);
    
    //Set up periodic clock
    timers[0].source = 1;
    timers[1].source = 2;

    epit_init(timers[0].reg);
    epit_init(timers[1].reg);

    //Start timer epit1 for timestamps updates
    epit_setTimerClock(timers[1].reg);
    epit_startTimer(timers[1].reg);

    queue.head = NULL;
    return 0;
}

/*
Functions to Manage hardware. Defines located in epit.h.

Init will clear Enabled, Input Overwrite and Output Mode.
It will enable Peripheral Clock, Enable mode 1 (LR or 0xFFFF_FFFF), interupts and Sets Reload mode

The Prescale is 3300, or 1 count every 0.00005 seconds.
*/

void epit_init(EPIT *timer){
    volatile uint32_t *CR = &(timer->REG_Control);

    //Set Prescaler to defined prescale value, set to periph clock, and enable reloading
    *CR |= (EPIT_PRESCALE_CONST | EPIT_CLK_PERIPHERAL | EPIT_RLD); 
    timer->REG_Status = 1; //Clear Status Register
    *CR |= (EPIT_EN_MOD | EPIT_OCI_EN); //Timer will reset from 0xFFFF_FFFF or Load Value
}

void epit_setTimerClock(EPIT *timer) {
    timer->REG_Control &= (0xFFFFFFFF ^ (EPIT_EN_MOD));
    timer->REG_Control &= (0xFFFFFFFF ^ (EPIT_RLD));
    timer->REG_Control |= EPIT_PRESCALE_MSK;
    timer->REG_Status = 1;
}

uint64_t epit_getCurrentTimestamp() {
    //Get number of ticks
    epit_stopTimer(timers[1].reg);
    uint64_t count = 0xFFFFFFFF - timers[1].reg->REG_Counter;
    epit_startTimer(timers[1].reg);

    //Convert to ms
    count *= EPIT_CLOCK_TICK;

    //Add overflow values
    uint64_t timestamp = timestamp_overflows * EPIT_CLOCK_OVERFLOW;

    timestamp = timestamp + count;
    return timestamp;
}

void epit_setTime(EPIT *timer, uint64_t milliseconds, int reset){
	//IF reset is non-zero We'll tell the timer to restart from our new value.
    if(reset){
		timer->REG_Control |= EPIT_I_OVW;	
    }

    uint32_t newCount;
    milliseconds *= EPIT_TICK_PER_MS;
    if(milliseconds > 0xFFFFFFFF) {
       newCount = 0xFFFFFFFF;
    } else {
       newCount = (uint32_t)milliseconds;
    } 
	timer->REG_Load = newCount;

    if(reset){
        timer->REG_Control &= (0xFFFFFFFF ^(EPIT_I_OVW));
    }
}

void epit_startTimer(EPIT *timer){
	timer->REG_Control |= EPIT_EN;
}

void epit_stopTimer(EPIT *timer){
	timer->REG_Control &= (0xFFFFFFFF ^ EPIT_EN);
}

int epit_timerRunning(EPIT *timer){
	return (timer->REG_Control & EPIT_EN);
}

uint32_t epit_getCount(EPIT *timer){
	return (timer->REG_Counter);
}

uint32_t epit_currentCompare(EPIT *timer){
	return (timer->REG_Compare);
}

int allocate_timer_id() {
    int i;
    for(i = 0; i < CLOCK_MAX_TIMERS; i++) {
        if(!freelist[i]) 
            return freelist[i];
    }
    return -1;
}

int deallocate_timer_id(int id) {
    if(freelist[id])
        freelist[id] = 0;
    return -1;
}
// On Failure, returns 0. Calling function must check!
uint32_t epit_register_callback(uint64_t delay, timer_callback_t callback, void *data) {
    //create node
    callback_node_t *node = malloc(sizeof(callback_node_t));
    if(!node){    
	dprintf(0, "Failed to Acquire Memory for Timer\n");
	    return 0;
    } 
    node->callback = callback;
    int id = allocate_timer_id();
    if(id == -1) {
        dprintf(0,"No Free Timer ID\n");
        free(node);
        return 0;
    }

    node->id = id;

    uint64_t timestamp = epit_getCurrentTimestamp() + delay;
    node->timestamp = timestamp;

    node->data = data;
    node->next = NULL;

    //if the timer is already activated
    if(queue.head) {
        //the current timer is before the new timer
       if(queue.head->timestamp <= timestamp) {
           callback_node_t *cur = queue.head;
           while(cur->next && cur->next->timestamp <= timestamp) {
                cur = cur->next;               
           }
           node->next = cur->next;
           cur->next = node;
        } else {
        //activate the new timer
            node->next = queue.head;
            queue.head = node;
            epit_stopTimer(timers[0].reg);
            epit_setTime(timers[0].reg, delay, 1);
            epit_startTimer(timers[0].reg);
        }
    } else {
        queue.head = node;
        epit_setTime(timers[0].reg, delay, 1);
        dprintf(0,"");
        epit_startTimer(timers[0].reg);
    }
            
    return id;
}
