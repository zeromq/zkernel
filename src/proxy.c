// Proxy manager class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

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
    unsigned int msgs_in_flight;
    unsigned long next_stream_id;
    bool stopped;
    msg_t *stop_msg;
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

    if (self->stopped) {
        session_destroy (&session);
        msg_destroy (&msg);
    }
    else {
        session_set_stream_id (session, self->next_stream_id++);
        msg->msg_type = ZKERNEL_START_IO;
        msg->u.start_io.io_object = (io_object_t *) session;
        msg->u.start_io.reply_to = self->actor_ifc;

        reactor_send (self->reactor, msg);
        self->msgs_in_flight++;
    }
}

static void
s_start_ack (proxy_t *self, msg_t *msg)
{
    io_object_t *io_object = msg->u.start_io_ack.io_object;

    *msg = (msg_t) {
        .msg_type = ZKERNEL_SESSION,
        .u.session = { .session = (session_t *) io_object },
    };

    self->msgs_in_flight--;
    actor_send (self->socket, msg);

    if (self->stopped && self->msgs_in_flight == 0) {
        assert (self->stop_msg);
        actor_send (self->socket, self->stop_msg);
    }
}

static void
s_start_nak (proxy_t *self, msg_t *msg)
{
    session_t *session = (session_t *) msg->u.start_io_nak.io_object;
    session_destroy (&session);
    msg_destroy (&msg);
    self->msgs_in_flight--;

    if (self->stopped && self->msgs_in_flight == 0) {
        assert (self->stop_msg);
        actor_send (self->socket, self->stop_msg);
    }
}

static void
s_stop (proxy_t *self, msg_t *msg)
{
    assert (!self->stopped);

    msg->msg_type = ZKERNEL_PROXY_STOPPED;

    if (self->msgs_in_flight == 0)
        actor_send (self->socket, msg);
    else
        self->stop_msg = msg;

    self->stopped = true;
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
        s_start_ack (self, msg);
        break;
    case ZKERNEL_START_IO_NAK:
        s_start_nak (self, msg);
        break;
    case ZKERNEL_STOP_PROXY:
        s_stop (self, msg);
        break;
    default:
        break;
    }
}
