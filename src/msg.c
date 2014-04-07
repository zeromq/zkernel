//  Message class

#include <stdlib.h>
#include <assert.h>

#include "msg.h"

msg_t *
msg_new (int cmd)
{
    msg_t *self = (msg_t *) malloc (sizeof *self);
    if (self)
        *self = (struct msg_t) { .cmd = cmd };
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

