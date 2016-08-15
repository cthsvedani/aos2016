#include "timer_tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void timerCallback(uint32_t id, void* data){
    register_timer(100, timerCallback, (void*)NULL);
    dprintf(0, "Good Morning Vietnam! time is %llu \n", epit_getCurrentTimestamp());
}
void timerCallbackz(uint32_t id, void* data){
    register_timer(250, timerCallbackz, (void*)NULL);
    dprintf(0, "Hello World, time is %llu \n", epit_getCurrentTimestamp());
}
