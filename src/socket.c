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

#include "reactor.h"
#include "tcp_listener.h"
#include "tcp_connector.h"
#include "tcp_session.h"
#include "mailbox.h"
#include "io_object.h"
#include "socket.h"
#include "atomic.h"
#include "msg.h"
#include "zkernel.h"

#define MAX_SESSIONS 8

struct socket {
    int ctrl_fd;
    struct mailbox reactor;
    void *mbox;
    tcp_session_t *sessions [MAX_SESSIONS];
    size_t current_session;
    size_t active_sessions;
    struct mailbox mailbox_ifc;
};

static int
    s_enqueue_msg (void *self_, struct msg_t *msg);

static msg_t *
    s_wait_for_msgs (socket_t *self);

static void
    process_mbox (socket_t *self, msg_t *msg);

static void
    s_new_session (socket_t *self, session_event_t *event);

static void
    s_session_closed (socket_t *self, session_closed_ev_t *ev);

static void
    s_ready_to_send (socket_t *self, ready_to_send_ev_t *ev);

socket_t *
socket_new (reactor_t *reactor)
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
        .reactor = reactor_mailbox (reactor),
        .mailbox_ifc = {
            .object = self,
            .ftab = { .enqueue = s_enqueue_msg }
        }
    };
    return self;
}

void
socket_destroy (socket_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        socket_t *self = *self_p;
        close (self->ctrl_fd);
        free (self);
        *self_p = NULL;
    }
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
    case ZKERNEL_REGISTER:
        msg_destroy (&msg);
        break;
    case ZKERNEL_REMOVE:
        msg_destroy (&msg);
        break;
    case ZKERNEL_NEW_SESSION:
        s_new_session (self, (session_event_t *) msg);
        msg_destroy (&msg);
        break;
    case ZKERNEL_SESSION_CLOSED:
        s_session_closed (self, (session_closed_ev_t *) msg);
        msg_destroy (&msg);
        break;
    case ZKERNEL_READY_TO_SEND:
        s_ready_to_send (self, (ready_to_send_ev_t *) msg);
        msg_destroy (&msg);
        break;
    default:
        printf ("unhandled message: %d\n", msg->msg_type);
        msg_destroy (&msg);
        break;
    }
}

int
socket_bind (socket_t *self, unsigned short port,
        protocol_engine_constructor_t *protocol_engine_constructor)
{
    tcp_listener_t *listener =
        tcp_listener_new (protocol_engine_constructor, &self->mailbox_ifc);
    if (!listener)
        goto fail;
    int rc = tcp_listener_bind (listener, port);
    if (rc == -1)
        goto fail;
    msg_t *msg = msg_new (ZKERNEL_REGISTER);
    if (!msg)
        goto fail;
    msg->reply_to = self->mailbox_ifc;
    msg->io_object = (io_object_t *) listener;
    mailbox_enqueue (&self->reactor, msg);
    return 0;

fail:
    if (listener)
        tcp_listener_destroy (&listener);
    return -1;
}

int
socket_connect (socket_t *self, unsigned short port,
        protocol_engine_constructor_t *protocol_engine_constructor)
{
    assert (self);

    tcp_connector_t *connector =
        tcp_connector_new (protocol_engine_constructor, &self->mailbox_ifc);
    if (!connector)
        return -1;
    const int rc = tcp_connector_connect (connector, port);
    if (rc != -1) {
        close (rc); //  close returned descriptor for now
        tcp_connector_destroy (&connector);
        return 0;
    }
    else
    if (tcp_connector_errno (connector) != EINPROGRESS) {
        tcp_connector_destroy (&connector);
        return rc;
    }
    msg_t *msg = msg_new (ZKERNEL_REGISTER);
    if (!msg) {
        tcp_connector_destroy (&connector);
        return -1;
    }
    msg->reply_to = self->mailbox_ifc;
    msg->io_object = (io_object_t *) connector;
    mailbox_enqueue (&self->reactor, msg);
    return 0;
}

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
        mailbox_enqueue (&self->reactor, msg);
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
s_new_session (socket_t *self, session_event_t *event)
{
    printf ("new session: %p\n", event->session);
    assert (self->active_sessions < MAX_SESSIONS);
    tcp_session_t *session = (tcp_session_t *) event->session;
    for (int i = self->active_sessions; i < MAX_SESSIONS; i++)
        if (self->sessions [i] == NULL) {
            self->sessions [i] = self->sessions [self->active_sessions];
            self->sessions [self->active_sessions++] = session;
            break;
        }
    msg_t *msg = msg_new (ZKERNEL_REGISTER);
    msg->reply_to = self->mailbox_ifc;
    msg->io_object = (io_object_t *) session;
    mailbox_enqueue (&self->reactor, msg);
}

static void
s_session_closed (socket_t *self, session_closed_ev_t *ev)
{
    printf ("session %p closed\n", ev->ptr);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (self->sessions [i] == ev->ptr) {
            if (i < self->active_sessions) {
                self->sessions [i] = self->sessions [self->active_sessions];
                self->sessions [self->active_sessions--] = NULL;
            }
            else
                self->sessions [i] = NULL;
            break;
        }
    }
}

static void
s_ready_to_send (socket_t *self, ready_to_send_ev_t *ev)
{
    printf ("session %p is ready to send data\n", ev->ptr);
    for (int i = self->active_sessions; i < MAX_SESSIONS; i++)
        if (self->sessions [i] == ev->ptr) {
            self->sessions [i] = self->sessions [self->active_sessions];
            self->sessions [self->active_sessions++] =
                (tcp_session_t *) ev->ptr;
            break;
        }
}
