#include "events.h"
#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define verbose 1
#include <sys/debug.h>
#include <sys/panic.h>
#include <sos.h>
extern s_EventHandlers *event_listeners[MAX_EVENTS];
extern n_Event *event_queue;

void init_events(s_EventHandlers *handlers[], int size){
    init_listeners(handlers, size);
}

void init_listeners(s_EventHandlers *handlers[], int size) {
    int i;
    for(i=0; i< MAX_EVENTS; i++){
        handlers[i] = NULL;
    }
}

void destroy_listeners(s_EventHandlers *handlers[], int size){
    int i;
    s_EventHandlers *tmp, *next;
    for( i=0; i<MAX_EVENTS; i++) {
        tmp = handlers[i];
        while(tmp) {
            next = tmp->next;
            free(tmp);
            tmp = next;
        } 
    }
}

int register_event(e_Events event, callback cb){
    s_EventHandlers *handlers = event_listeners[event];

    if(handlers == NULL){
        handlers = malloc(sizeof(s_EventHandlers));
        if(!handlers) {
            panic("Nomem: register_events");
        }

        handlers->cb = cb;
        handlers->next = NULL;
        event_listeners[event] = handlers;
    } else {
        while(!handlers->next){ 
            if(handlers->cb == cb) {
                return -1;
            }
            handlers = handlers->next;
        }

        s_EventHandlers *next_handler;
        next_handler = malloc(sizeof(s_EventHandlers));
        if(!next_handler){
            panic("Nomem: register_events");
        }

        next_handler->cb = cb;
        next_handler->next = NULL;
        handlers->next = next_handler;
    }

    return 1;
}

void notify(e_Events event, void *data){
    s_EventHandlers *handlers = event_listeners[event];
    if(!handlers){
        return;
    }

    while(handlers) {
        handlers->cb(data);
        handlers = handlers->next;
    }
}

void emit(e_Events event, void *data){ 
    n_Event *event_node;
    event_node = malloc(sizeof(n_Event));
    if(!event_node) {
        panic("Nomem: eq_add");
    }
    event_node->event = event;
    event_node->data = data;
    event_node->next = NULL;
    eq_add(event_node);
}

void eq_add(n_Event *event_node) {
    if(!event_queue) {
        event_queue = event_node;
    } else {
        n_Event *tmp = event_queue;
        while(tmp->next) {
            tmp = tmp->next;
        }

        tmp->next = event_node;
    }
}

n_Event* eq_pop(){
    if(!event_queue){
        return NULL;
    } else {
        n_Event *tmp;
        tmp = event_queue;
        event_queue = event_queue->next;
        return tmp;
    }
}

void handle_next_event(){
    n_Event *next;
    next = eq_pop();
    if(!next) {
        return; 
    }
    
    e_Events event = next->event;
    void* data = next->data;
    free(next);

    notify(event, data); 
}
