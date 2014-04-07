// Socket class

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <assert.h>
#include <errno.h>

#include "reactor.h"
#include "tcp_listener.h"
#include "mailbox.h"
#include "socket.h"
#include "atomic.h"
#include "msg.h"

struct socket {
    int ctrl_fd;
    struct mailbox reactor;
    void *mbox;
    struct mailbox mailbox_ifc;
};

static int
    s_enqueue_msg (void *self_, struct msg_t *msg);

static int
    s_wait_for_reply (socket_t *self, struct mailbox *peer);

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
        //  tell reactor we are leaving
        free (self);
        *self_p = NULL;
    }
}

int
socket_bind (socket_t *self, unsigned short port)
{
    tcp_listener_t *listener = tcp_listener_new (&self->mailbox_ifc);
    if (!listener)
        goto fail;
    int rc = tcp_listener_bind (listener, port);
    if (rc == -1)
        goto fail;
    msg_t *msg = msg_new (ZKERNEL_REGISTER);
    if (!msg)
        goto fail;
    msg->reply_to = self->mailbox_ifc;
    msg->fd = tcp_listener_fd (listener);
    msg->handler = tcp_listener_io_handler (listener);

    mailbox_enqueue (&self->reactor, msg);
    s_wait_for_reply (self, &self->reactor);
    return 0;

fail:
    if (listener)
        tcp_listener_destroy (&listener);
    return -1;
}

void
socket_noop (socket_t *self)
{
    assert (self);
    void *ptr = atomic_ptr_swap (&self->mbox, NULL);
    struct msg_t *msg = (struct msg_t *) ptr;
    while (msg) {
        printf ("message received\n");
        struct msg_t *next_msg = msg->next;
        msg_destroy (&msg);
        msg = next_msg;
    }
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

static int
s_wait_for_reply (socket_t *self, struct mailbox *peer)
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
    struct msg_t *msg = (struct msg_t *) ptr;
    msg_destroy (&msg);
    return 0;
}
