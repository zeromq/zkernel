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
#include "session.h"
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

static void
s_session (proxy_t *self, msg_t *msg)
{
    io_object_t *session = msg->u.session.session;
    assert (session);

    msg_t *msg2 = msg_new (ZKERNEL_START_IO);
    if (msg2) {
        actor_send (self->socket, msg);

        msg2->u.start_io.io_object = session;
        msg2->u.start_io.reply_to = self->actor_ifc;
        reactor_send (self->reactor, msg2);
    }
    else {
        io_object_destroy (&session);
        msg_destroy (&msg);
    }
}

static void
s_start_io_ack (proxy_t *self, msg_t *msg)
{
    actor_send (self->socket, msg);
}

static void
s_start_io_nak (proxy_t *self, msg_t *msg)
{
    actor_send (self->socket, msg);
}

void
proxy_message (proxy_t *self, msg_t *msg)
{
    assert (self);

    switch (msg->msg_type) {
    case ZKERNEL_SESSION:
        s_session (self, msg);
        break;
    case ZKERNEL_START_IO_ACK:
        s_start_io_ack (self, msg);
        break;
    case ZKERNEL_START_IO_NAK:
        s_start_io_nak (self, msg);
        break;
    default:
        break;
    }
}
