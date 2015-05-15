// Proxy manager class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __PROXY_H_INCLUDED__
#define __PROXY_H_INCLUDED__

#include "msg.h"
#include "actor.h"
#include "dispatcher.h"
#include "reactor.h"
#include "zkernel.h"

typedef struct proxy proxy_t;

proxy_t *
    proxy_new (actor_t *socket, io_descriptor_t *(*session_alocator) (), dispatcher_t *dispatcher, reactor_t *reactor);

void
    proxy_send (proxy_t *self, msg_t *msg);

void
    proxy_destroy (proxy_t **self_p);

#endif
