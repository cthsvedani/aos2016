#ifndef _EVENT_H_
#define _EVENT_H_

typedef enum Events{
    GETDIRENT_COMPLETE,
    READ_COMPLETE,
    WRITE_COMPLETE,
    MAX_EVENTS
}e_Events;

typedef struct Event_node {
    e_Events event;
    void *data;
    struct Event_node *next;
} n_Event;

typedef void (*callback) (void *);

typedef struct EventHandlers{
        callback cb;
        struct EventHandlers *next;
} s_EventHandlers;

s_EventHandlers *listeners[MAX_EVENTS];

//event handling
void handle_next_event();
void notify(e_Events event, void *data);

//event handlers
void init_events();
void init_listeners(s_EventHandlers *handlers[], int size);
void destroy_listeners(s_EventHandlers *handlers[], int size);
int register_event(e_Events event, callback cb);
void test_events();

void handle_test1(void *data);
void handle_test2(void *data);
void handle_test3(void *data);

//event queue
void emit(e_Events event, void *data);
void eq_add(n_Event *event_node);
n_Event* eq_pop();

#endif
