//  Event handler base class

#include "event_handler.h"

void
event_handler_set_id (event_handler_t *self, void *handler_id)
{
    self->handler_id = handler_id;
}

void *
event_handler_id (event_handler_t *self)
{
    return self->handler_id;
}
