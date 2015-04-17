//  Message dispatcher class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __DISPATCHER_H_INCLUDED__
#define __DISPATCHER_H_INCLUDED__

#include "msg.h"

typedef struct dispatcher dispatcher_t;

dispatcher_t *
    dispatcher_new ();

void
    dispatcher_destroy (dispatcher_t **self_p);

void
    dispatcher_send (dispatcher_t *self, msg_t *msg);

#endif

