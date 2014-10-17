//  TCP listener class

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mailbox.h"
#include "tcp_listener.h"
#include "tcp_session.h"
#include "msg.h"
#include <arpa/inet.h>
#include <errno.h>
#include "zkernel.h"
#include "event_handler.h"

struct tcp_listener {
    event_handler_t base;
    int fd;
    decoder_constructor_t *decoder_constructor;
    mailbox_t *owner;
};

tcp_listener_t *
tcp_listener_new (decoder_constructor_t *decoder_constructor, mailbox_t *owner)
{
    tcp_listener_t *self = malloc (sizeof *self);
    if (self)
        *self = (tcp_listener_t) { .fd = -1, .decoder_constructor = decoder_constructor, .owner = owner };
    return self;
}

void
tcp_listener_destroy (tcp_listener_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        tcp_listener_t *self = *self_p;
        if (self->fd != -1)
            close (self->fd);
        free (self);
        *self_p = NULL;
    }
}

int
tcp_listener_bind (tcp_listener_t *self, unsigned short port)
{
    int rc;

    assert (self);
    if (self->fd != -1)
        return -1;
    const int fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;
    const int on = 1;
    //  Allow port reuse
    rc = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
    assert (rc == 0);

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons (port),
        .sin_addr.s_addr = htonl (INADDR_ANY)
    };
    rc = bind (fd, (struct sockaddr *) &server_addr, sizeof server_addr);
    if (rc == -1) {
        close (fd);
        return -1;
    }
    rc = listen (fd, 32);
    if (rc == -1) {
        close (fd);
        return -1;
    }

    self->fd = fd;
    return 0;
}

static int
io_init (void *self_, int *fd, uint32_t *timer_interval)
{
    tcp_listener_t *self = (tcp_listener_t *) self_;
    assert (self);

    //  Set non-blocking mode
    const int flags = fcntl (self->fd, F_GETFL, 0);
    assert (flags != -1);
    int rc = fcntl (self->fd, F_SETFL, flags | O_NONBLOCK);
    assert (rc == 0);

    *fd = self->fd;
    return 3;
}

static int
io_event (void *self_, uint32_t flags, int *fd, uint32_t *timer_interval)
{
    tcp_listener_t *self = (tcp_listener_t *) self_;
    assert (self);

    if ((flags & ZKERNEL_IO_ERROR) == ZKERNEL_IO_ERROR) {
        printf ("tcp_listener: I/O error\n");
        *fd = -1;
        return 0;
    }

    while (1) {
        const int rc = accept (self->fd, NULL, NULL);
        if (rc == -1) {
            assert (errno == EAGAIN);
            break;
        }
        printf ("connection accepted\n");

        tcp_session_t *session = tcp_session_new (rc, self->decoder_constructor, self->owner);
        if (!session) {
            close (rc);
            continue;
        }
        session_event_t *ev = session_event_new ();
        if (ev == NULL) {
            tcp_session_destroy (&session);
            continue;
        }
        ev->ptr = session;
        mailbox_enqueue (self->owner, (msg_t *) ev);
    }
    return 1 | 2;
}

struct io_handler
tcp_listener_io_handler (tcp_listener_t *self)
{
    static struct io_handler_ops ops = {
        .init  = io_init,
        .event = io_event
    };
    assert (self);
    return (struct io_handler) { .object = self, .ops = &ops };
}
