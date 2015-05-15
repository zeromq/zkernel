// Socket class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <assert.h>
#include <errno.h>

#include "dispatcher.h"
#include "reactor.h"
#include "proxy.h"
#include "io_object.h"
#include "socket.h"
#include "atomic.h"
#include "msg.h"
#include "zkernel.h"

#define MAX_SESSIONS 8

struct socket {
    int ctrl_fd;
    reactor_t *reactor;
    proxy_t *proxy;
    void *mbox;
    unsigned long listener_next_id;
    unsigned long connector_next_id;
    size_t current_session;
    size_t active_sessions;
    struct actor actor_ifc;
};

struct session {
    io_descriptor_t base;
    io_object_t *io_object;
};

struct listener {
    io_descriptor_t base;
    io_object_t *io_object;
};

struct connector {
    io_descriptor_t base;
    io_object_t *io_object;
};

static int
    s_enqueue_msg (void *self_, struct msg_t *msg);

static msg_t *
    s_wait_for_msgs (socket_t *self);

static void
    process_mbox (socket_t *self, msg_t *msg);

static void
    s_session (socket_t *self, msg_t *msg);

static void
    s_session_closed (socket_t *self, msg_t *msg);

static io_descriptor_t *
s_new_session ()
{
    struct session *session =
        (struct session *) malloc (sizeof *session);
    if (session)
        *session = (struct session) { .io_object = NULL };
    return &session->base;
}

static void *
s_new_listener ()
{
    struct listener *listener =
        (struct listener *) malloc (sizeof *listener);
    if (listener)
        *listener = (struct listener) { .io_object = NULL };
    return listener;
}

static struct connector *
s_new_connector ()
{
    struct connector *connector =
        (struct connector *) malloc (sizeof *connector);
    if (connector)
        *connector = (struct connector) { .io_object = NULL };
    return connector;
}

socket_t *
socket_new (dispatcher_t *dispatcher, reactor_t *reactor)
{
    socket_t *self = malloc (sizeof *self);
    if (!self)
        return NULL;
    //  XXX EFD_CLOEXEC only since Linux 2.6.27
    int ctrl_fd = eventfd (0, EFD_CLOEXEC);
    if (ctrl_fd == -1) {
        free (self);
        return NULL;
    }
    *self = (socket_t) {
        .ctrl_fd = ctrl_fd,
        .reactor = reactor,
        .actor_ifc = {
            .object = self,
            .ftab = { .send = s_enqueue_msg }
        }
    };
    self->proxy = proxy_new (
        &self->actor_ifc, s_new_session, dispatcher, reactor);
    if (self->proxy == NULL) {
        close (self->ctrl_fd);
        free (self);
        self = NULL;
    }
    return self;
}

void
socket_destroy (socket_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        socket_t *self = *self_p;
        close (self->ctrl_fd);
        proxy_destroy (&self->proxy);
        free (self);
        *self_p = NULL;
    }
}

proxy_t *
socket_proxy (socket_t *self)
{
    return self->proxy;
}

static void
process_msg (socket_t *self, msg_t **msg_p)
{
    assert (msg_p);
    if (*msg_p == NULL)
        return;
    msg_t *msg = *msg_p;
    *msg_p = NULL;

    switch (msg->msg_type) {
    case ZKERNEL_MSG_TYPE_PDU:
        msg_destroy (&msg);
        break;
    case ZKERNEL_SESSION:
        s_session (self, msg);
        msg_destroy (&msg);
        break;
    case ZKERNEL_SESSION_CLOSED:
        s_session_closed (self, msg);
        msg_destroy (&msg);
        break;
    default:
        printf ("unhandled message: %d\n", msg->msg_type);
        msg_destroy (&msg);
        break;
    }
}

int
socket_listen (socket_t *self, io_object_t *io_object)
{
    assert (self);

    msg_t *msg = msg_new (ZKERNEL_START_IO);
    if (!msg)
        return -1;
    struct listener *listener = s_new_listener ();
    if (listener == NULL) {
        msg_destroy (&msg);
        return -1;
    }

    msg->u.start_io.io_object = io_object;
    msg->u.start_io.io_descriptor = &listener->base;
    msg->u.start_io.reply_to = self->actor_ifc;

    reactor_send (self->reactor, msg);

    return 0;
}

int
socket_connect (socket_t *self, io_object_t *io_object)
{
    assert (self);

    msg_t *msg = msg_new (ZKERNEL_START_IO);
    if (!msg)
        return -1;
    struct connector *connector = s_new_connector ();
    if (connector == NULL) {
        msg_destroy (&msg);
        return -1;
    }

    msg->u.start_io.io_object = io_object;
    msg->u.start_io.io_descriptor = &connector->base;
    msg->u.start_io.reply_to = self->actor_ifc;

    reactor_send (self->reactor, msg);

    return 0;
}

void
socket_send_msg (socket_t *self, msg_t *msg)
{
    assert (self);
    s_enqueue_msg (self, msg);
}

/*
int
socket_send (socket_t *self, const char *data, size_t size)
{
    assert (self);
    if (self->active_sessions == 0)
        return -1;
    tcp_session_t *session = self->sessions [self->current_session];
    assert (session);
    const int rc = tcp_session_send (session, data, size);
    if (rc == -1) {
        msg_t *msg = msg_new (ZKERNEL_ACTIVATE);
        assert (msg);
        msg->event_mask = ZKERNEL_POLLOUT;
        reactor_send (self->reactor, msg);
        self->active_sessions--;
        if (self->current_session < self->active_sessions) {
            self->sessions [self->current_session] =
                self->sessions [self->active_sessions];
            self->sessions [self->active_sessions] = session;
        }
    }
    else
        self->current_session++;
    if (self->current_session >= self->active_sessions)
        self->current_session = 0;
    return 0;
}
*/

static void
process_mbox (socket_t *self, msg_t *msg)
{
    //  Transform LIFO to FIFO
    msg_t *prev = NULL;
    while (msg->next) {
        msg_t *next = msg->next;
        msg->next = prev;
        prev = msg;
        msg = next;
    }
    msg->next = prev;
    while (msg) {
        msg_t *next = msg->next;
        process_msg (self, &msg);
        msg = next;
    }
}

void
socket_noop (socket_t *self)
{
    assert (self);
    void *ptr = atomic_ptr_swap (&self->mbox, NULL);
    if (ptr)
        process_mbox (self, (msg_t *) ptr);
}

static int
s_enqueue_msg (void *self_, struct msg_t *msg)
{
    socket_t *self = (socket_t *) self_;
    assert (self);
    void *tail = atomic_ptr_get (&self->mbox);
    atomic_ptr_set ((void **) &msg->next, tail == self? NULL: tail);
    void *prev = atomic_ptr_cas (&self->mbox, tail, msg);
    while (prev != tail) {
        tail = prev;
        atomic_ptr_set ((void **) &msg->next, tail);
        prev = atomic_ptr_cas (&self->mbox, tail, msg);
    }
    if (prev == self) {
        uint64_t v = 1;
        const int rc = write (self->ctrl_fd, &v, sizeof v);
        assert (rc == sizeof v);
    }
    return 0;
}

static msg_t *
s_wait_for_msgs (socket_t *self)
{
    void *ptr = atomic_ptr_swap (&self->mbox, NULL);
    if (ptr == NULL) {
        ptr = atomic_ptr_cas (&self->mbox, NULL, self);
        if (ptr == NULL) {
            uint64_t buf;
            int rc = read (self->ctrl_fd, &buf, sizeof buf);
            while (rc == -1) {
                assert (errno == EINTR);
                rc = read (self->ctrl_fd, &buf, sizeof buf);
            }
            assert (rc == sizeof buf);
            ptr = atomic_ptr_swap (&self->mbox, NULL);
            assert (ptr);
        }
    }
    return (struct msg_t *) ptr;
}

static void
s_session (socket_t *self, msg_t *msg)
{
    printf ("new session: %p\n", msg->u.session.session);

    struct session *session =
        (struct session *) msg->u.session.io_descriptor;
    session->io_object = msg->u.session.session;
}

static void
s_session_closed (socket_t *self, msg_t *msg)
{
}
