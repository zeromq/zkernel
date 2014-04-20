// Socket class

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
#include "socket.h"
#include "atomic.h"
#include "msg.h"
#include "set.h"
#include "zkernel.h"

struct event_handler {
    void *handler_id;
};

struct socket {
    int ctrl_fd;
    struct mailbox reactor;
    void *mbox;
    zset_t *event_handlers;
    struct mailbox mailbox_ifc;
};

static int
    s_enqueue_msg (void *self_, struct msg_t *msg);

static msg_t *
    s_wait_for_msgs (socket_t *self);

static void
    process_mbox (socket_t *self, msg_t *msg);

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
    zset_t *event_handlers = zset_new ();
    if (!event_handlers) {
        close (ctrl_fd);
        free (self);
        return NULL;
    }
    *self = (socket_t) {
        .ctrl_fd = ctrl_fd,
        .reactor = reactor_mailbox (reactor),
        .event_handlers = event_handlers,
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
        struct event_handler *ev_handler =
            (struct event_handler *) zset_first (self->event_handlers);
        while (ev_handler) {
            while (ev_handler->handler_id == NULL)
                process_mbox (self, s_wait_for_msgs (self));
            msg_t *msg = msg_new (ZKERNEL_REMOVE);
            msg->reply_to = self->mailbox_ifc;
            msg->ptr = ev_handler;
            msg->handler_id = ev_handler->handler_id;
            mailbox_enqueue (&self->reactor, msg);
            ev_handler =
                (struct event_handler *) zset_next (self->event_handlers);
        }
        while (zset_non_empty (self->event_handlers))
            process_mbox (self, s_wait_for_msgs (self));
        zset_destroy (&self->event_handlers);
        close (self->ctrl_fd);
        free (self);
        *self_p = NULL;
    }
}

static void
process_msg (socket_t *self, msg_t **msg_p)
{
    tcp_session_t *session;

    assert (msg_p);
    if (*msg_p == NULL)
        return;
    msg_t *msg = *msg_p;
    *msg_p = NULL;

    switch (msg->cmd) {
    case ZKERNEL_REGISTER:
        assert (msg->ptr);
        ((struct event_handler *) msg->ptr)->handler_id = msg->handler_id;
        msg_destroy (&msg);
        break;
    case ZKERNEL_REMOVE:
        assert (msg->ptr);
        zset_remove (self->event_handlers, msg->ptr);
        free (msg->ptr);
        msg_destroy (&msg);
        break;
    case ZKERNEL_NEW_SESSION: {
        printf ("new session: %p\n", msg->ptr);
        struct event_handler *event_handler = (struct event_handler *)
            malloc (sizeof *event_handler);
        assert (event_handler);
        *event_handler = (struct event_handler) { .handler_id = NULL };
        session = (tcp_session_t *) msg->ptr;
        msg->cmd = ZKERNEL_REGISTER;
        msg->reply_to = self->mailbox_ifc;
        msg->handler = tcp_session_io_handler (session);
        msg->ptr = event_handler;
        mailbox_enqueue (&self->reactor, msg);
        zset_add (self->event_handlers, event_handler);
        break;
                              }
    case ZKERNEL_SESSION_CLOSED:
        printf ("session %p closed\n", msg->ptr);
        msg_destroy (&msg);
        break;
    case ZKERNEL_READY_TO_SEND:
        printf ("session %p is ready to send data\n", msg->ptr);
        msg_destroy (&msg);
        break;
    default:
        printf ("unhandled message: %d\n", msg->cmd);
        msg_destroy (&msg);
        break;
    }
}

int
socket_bind (socket_t *self, unsigned short port)
{
    tcp_listener_t *listener = tcp_listener_new (&self->mailbox_ifc);
    struct event_handler *event_handler = NULL;
    if (!listener)
        goto fail;
    int rc = tcp_listener_bind (listener, port);
    if (rc == -1)
        goto fail;
    event_handler = (struct event_handler *)
        malloc (sizeof *event_handler);
    if (!event_handler)
        goto fail;
    *event_handler = (struct event_handler) { .handler_id = NULL };
    msg_t *msg = msg_new (ZKERNEL_REGISTER);
    if (!msg)
        goto fail;
    msg->reply_to = self->mailbox_ifc;
    msg->handler = tcp_listener_io_handler (listener);
    msg->ptr = event_handler;
    mailbox_enqueue (&self->reactor, msg);
    zset_add (self->event_handlers, event_handler);
    return 0;

fail:
    if (listener)
        tcp_listener_destroy (&listener);
    if (event_handler)
        free (event_handler);
    return -1;
}

int
socket_connect (socket_t *self, unsigned short port)
{
    assert (self);

    tcp_connector_t *connector =
        tcp_connector_new (&self->mailbox_ifc);
    struct event_handler *event_handler = NULL;
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

    event_handler = (struct event_handler *)
        malloc (sizeof *event_handler);
    if (!event_handler) {
        tcp_connector_destroy (&connector);
        return -1;
    }
    *event_handler = (struct event_handler) { .handler_id = NULL };
    msg_t *msg = msg_new (ZKERNEL_REGISTER);
    if (!msg) {
        free (event_handler);
        tcp_connector_destroy (&connector);
        return -1;
    }
    msg->reply_to = self->mailbox_ifc;
    msg->handler = tcp_connector_io_handler (connector);
    msg->ptr = event_handler;
    mailbox_enqueue (&self->reactor, msg);
    zset_add (self->event_handlers, event_handler);
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
