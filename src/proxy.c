// Proxy manager class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "msg.h"
#include "actor.h"
#include "dispatcher.h"
#include "reactor.h"
#include "proxy.h"
#include "zkernel.h"

struct proxy {
    actor_t *socket;
    dispatcher_t *dispatcher;
    reactor_t *reactor;
    struct actor actor_ifc;
};

static int
    s_enqueue_msg (void *self_, struct msg_t *msg);

proxy_t *
proxy_new (actor_t *socket, dispatcher_t *dispatcher, reactor_t *reactor)
{
    proxy_t *self = (proxy_t *) malloc (sizeof *self);
    if (self) {
        *self = (proxy_t) {
            .socket = socket,
            .dispatcher = dispatcher,
            .reactor = reactor,
            .actor_ifc = {
                .object = self,
                .ftab = { .send = s_enqueue_msg },
            },
        };
    }

    return self;
}

void
proxy_destroy (proxy_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        proxy_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}

int
s_enqueue_msg (void *self_, msg_t *msg)
{
    proxy_t *self = (proxy_t *) self_;
    msg->proxy = self;
    dispatcher_send (((proxy_t *) self)->dispatcher, msg);
}

void
proxy_send (proxy_t *self, msg_t *msg)
{
    dispatcher_send (self->dispatcher, msg);
}

void
proxy_message (proxy_t *self, msg_t *msg)
{
    assert (self);

    switch (msg->msg_type) {
    case ZKERNEL_LISTENER:
        break;
    case ZKERNEL_LISTENER_ERROR:
        break;
    case ZKERNEL_LISTENER_TERMINATED:
        break;
    case ZKERNEL_CONNECTOR:
        break;
    case ZKERNEL_CONNECTOR_ERROR:
        break;
    case ZKERNEL_CONNECTOR_TERMINATED:
        break;
    case ZKERNEL_SESSION:
        break;
    case ZKERNEL_SESSION_ERROR:
        break;
    case ZKERNEL_SESSION_TERMINATED:
        break;
    default:
        break;
    }
    msg_destroy (&msg);
}
