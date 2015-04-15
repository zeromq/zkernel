// Message queue class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "msg.h"
#include "msg_queue.h"

struct msg_queue {
    msg_t *front;
    msg_t *back;
};

msg_queue_t *
msg_queue_new ()
{
    msg_queue_t *self = (msg_queue_t *) malloc (sizeof *self);
    if (self)
        *self = (msg_queue_t) { .front = NULL };

    return self;
}

void
msg_queue_enqueue (msg_queue_t *self, msg_t *msg)
{
    if (self->back == NULL)
        self->front = self->back = msg;
    else {
        self->back->next = msg;
        self->back = msg;
    }

    msg->next = NULL;
}

msg_t *
msg_queue_dequeue (msg_queue_t *self)
{
    msg_t *msg;

    if ((msg = self->front) != NULL)
        if ((self->front = msg->next) == NULL)
            self->back = NULL;
    msg->next = NULL;

    return msg;
}

bool
msg_queue_is_empty (msg_queue_t *self)
{
    return self->front == NULL;
}

void
msg_queue_destroy (msg_queue_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        msg_queue_t *self = *self_p;
        while (!msg_queue_is_empty (self)) {
            msg_t *msg = msg_queue_dequeue (self);
            msg_destroy (&msg);
        }
        free (self);
        *self_p = NULL;
    }
}
