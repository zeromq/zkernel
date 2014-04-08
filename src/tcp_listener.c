//  TCP listener class

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "mailbox.h"
#include "tcp_listener.h"
#include "tcp_session.h"
#include "msg.h"
#include <arpa/inet.h>
#include <errno.h>

struct tcp_listener {
    int fd;
    mailbox_t *owner;
};

static int
    io_event (void *self_, uint32_t flags);

static void
    io_error (void *self_);

tcp_listener_t *
tcp_listener_new (mailbox_t *owner)
{
    tcp_listener_t *self = malloc (sizeof *self);
    if (self)
        *self = (tcp_listener_t) { .fd = -1, .owner = owner };
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
    assert (self);
    if (self->fd != -1)
        return -1;
    const int fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return -1;
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons (port),
        .sin_addr.s_addr = htonl (INADDR_ANY)
    };
    int rc = bind
        (fd, (struct sockaddr *) &server_addr, sizeof server_addr);
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

int
tcp_listener_fd (tcp_listener_t *self)
{
    assert (self);
    return self->fd;
}

struct io_handler
tcp_listener_io_handler (tcp_listener_t *self)
{
    static struct io_handler_ops ops = {
        .event = io_event,
        .error = io_error
    };
    assert (self);
    return (struct io_handler) { .object = self, .ops = &ops };
}

static int
io_event (void *self_, uint32_t flags)
{
    tcp_listener_t *self = (tcp_listener_t *) self_;
    assert (self);
    while (1) {
        const int rc = accept (self->fd, NULL, NULL);
        if (rc == -1) {
            assert (errno == EAGAIN);
            break;
        }
        printf ("connection accepted\n");

        tcp_session_t *session = tcp_session_new (rc);
        if (!session) {
            close (rc);
            continue;
        }
        msg_t *msg = msg_new (ZKERNEL_EVENT_NEW_SESSION);
        if (!msg) {
            tcp_session_destroy (&session);
            continue;
        }
        msg->ptr = session;
        mailbox_enqueue (self->owner, msg);
    }
    return 1 | 2;
}

static void
io_error (void *self_)
{
    tcp_listener_t *self = (tcp_listener_t *) self_;
    assert (self);
    printf ("I/O error\n");
}
