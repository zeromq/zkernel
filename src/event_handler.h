//  Event handler base class

#ifndef __EVENT_HANDLER_H_INCLUDED__
#define __EVENT_HANDLER_H_INCLUDED__

struct event_handler {
    void *handler_id;
};

typedef struct event_handler event_handler_t;

void
    event_handler_set_id (event_handler_t *self, void *handler_id);

void *
    event_handler_id (event_handler_t *self);

#endif
