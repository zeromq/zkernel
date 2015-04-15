//  Message queue class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __MSG_QUEUE_H_INCLUDED__
#define __MSG_QUEUE_H_INCLUDED__

#include <stdbool.h>

#include "msg.h"

typedef struct msg_queue msg_queue_t;

msg_queue_t *
    msg_queue_new ();

void
    msg_queue_enqueue (msg_queue_t *self, msg_t *msg);

msg_t *
    msg_queue_dequeue (msg_queue_t *self);

bool
    msg_queue_is_empty (msg_queue_t *self);

void
    msg_queue_destroy (msg_queue_t **self_p);

#endif
