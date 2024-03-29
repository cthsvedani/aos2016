/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <sel4/sel4.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <clock/epit.h>
#include <clock/clock.h>

/*
 * Return codes for driver functions
 */
#define CLOCK_R_OK     0        /* success */
#define CLOCK_R_UINT (-1)       /* driver not initialised */
#define CLOCK_R_CNCL (-2)       /* operation cancelled (driver stopped) */
#define CLOCK_R_FAIL (-3)       /* operation failed for other reason */
#define CLOCK_MAX_TIMERS 32

typedef struct callback_node_t callback_node_t;
struct callback_node_t
{ 
    callback_node_t *next;
    timer_callback_t callback;
    void* data;
    int id;
    uint64_t timestamp;
};

typedef struct {
    callback_node_t *head;
} callback_queue;

/*
 * Register callback timer, add to the linked list of callbacks, change timer if necessary
 */

/*
 * Get the next free identifier
 */
int allocate_timer_id();

/*
 * Deallocate free identifier
 */
int deallocate_timer_id(int id);


/*
 * Initialise driver. Performs implicit stop_timer() if already initialised.
 *    interrupt_ep:       A (possibly badged) async endpoint that the driver
                          should use for deliverying interrupts to
 *
 * Returns CLOCK_R_OK iff successful.
 */
int start_timer(seL4_CPtr interrupt_ep);

/*
 * Register a callback to be called after a given delay
 *    delay:  Delay time in microseconds before callback is invoked
 *    callback: Function to be called
 *    data: Custom data to be passed to callback function
 *
 * Returns 0 on failure, otherwise an unique ID for this timeout
 */
uint32_t register_timer(uint64_t delay, timer_callback_t callback, void *data);

/*
 * Remove a previously registered callback by its ID
 *    id: Unique ID returned by register_time
 * Returns CLOCK_R_OK iff successful.
 */
int remove_timer(uint32_t id);

/*
 * Handle an interrupt message sent to 'interrupt_ep' from start_timer
 *
 * Returns CLOCK_R_OK iff successful
 */
int timer_interrupt(void);

/*
 * Returns present time in microseconds since booting.
 *
 * Returns a negative value if failure.
 */
timestamp_t time_stamp(void);

/*
 * Stop clock driver operation.
 *
 * Returns CLOCK_R_OK iff successful.
 */
int stop_timer(void);
void handle_irq(timer timer);
void handle_irq_callback();

void clock_irq(void);

#endif /* _TIMER_H_ */
