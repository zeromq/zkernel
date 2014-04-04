// Socket class

#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <assert.h>

#include "reactor.h"
#include "tcp_listener.h"
#include "mailbox.h"
#include "socket.h"

struct socket {
    int ctrl_fd;
    struct mailbox reactor;
};

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
        .reactor = reactor_mailbox (reactor)
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
    tcp_listener_t *listener = tcp_listener_new ();
    if (!listener)
        goto fail;
    int rc = tcp_listener_bind (listener, port);
    if (rc == -1)
        goto fail;
    struct msg_t *msg = malloc (sizeof *msg);
    if (!msg)
        goto fail;
    *msg = (struct msg_t) {
        .cmd = ZKERNEL_BIND,
        .fd = tcp_listener_fd (listener),
        .handler = tcp_listener_io_handler (listener)
    };
    mailbox_enqueue (&self->reactor, msg);
    //  wait for response
    return 0;

fail:
    if (listener)
        tcp_listener_destroy (&listener);
    return -1;
}
