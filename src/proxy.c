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
    unsigned long next_session_id;
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
    session_t *session = msg->u.session.session;
    assert (session);

    msg_t *msg2 = msg_new (ZKERNEL_START_IO);
    if (msg2) {
        const unsigned long session_id =
            self->next_session_id++;
        session_set_session_id (session, session_id);
        msg->u.session.session_id = session_id;
        actor_send (self->socket, msg);

        msg2->u.start_io.object_id = session_id;
        msg2->u.start_io.io_object = (io_object_t *) session;
        msg2->u.start_io.reply_to = *self->socket;
        reactor_send (self->reactor, msg2);
    }
    else {
        session_destroy (&session);
        msg_destroy (&msg);
    }
}

void
proxy_message (proxy_t *self, msg_t *msg)
{
    assert (self);

    switch (msg->msg_type) {
    case ZKERNEL_SESSION:
        s_session (self, msg);
        break;
    default:
        break;
    }
}
