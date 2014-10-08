//  Message class

#include <stdlib.h>
#include <assert.h>

#include "msg.h"
#include "zkernel.h"

msg_t *
msg_new (int msg_type)
{
    msg_t *self = (msg_t *) malloc (sizeof *self);
    if (self)
        *self = (struct msg_t) { .msg_type = msg_type };
    return self;
}

void
msg_destroy (msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        msg_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}

session_event_t *
session_event_new ()
{
    session_event_t *ev = (session_event_t *) malloc (sizeof *ev);
    if (ev != NULL)
        *ev = (session_event_t) { .base.msg_type = ZKERNEL_NEW_SESSION };
    return ev;
}
