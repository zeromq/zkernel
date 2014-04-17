//  TCP session class

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "tcp_session.h"
#include "msg.h"
#include "zkernel.h"

struct tcp_session {
    int fd;
    mailbox_t *owner;
};

tcp_session_t *
tcp_session_new (int fd, mailbox_t *owner)
{
    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self)
        *self = (tcp_session_t) { .fd = fd, .owner = owner };
    return self;
}

void
tcp_session_destroy (tcp_session_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        tcp_session_t *self = *self_p;
        if (self->fd != -1)
            close (self->fd);
        free (self);
        *self_p = NULL;
    }
}

static int
io_init (void *self_, int *fd, uint32_t *timer_interval)
{
    tcp_session_t *self = (tcp_session_t *) self_;
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
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    if ((flags & ZKERNEL_IO_ERROR) == ZKERNEL_IO_ERROR) {
        printf ("tcp_session: I/O error\n");
        *fd = -1;
        return 0;
    }

    if ((flags & ZKERNEL_INPUT_READY) == ZKERNEL_INPUT_READY) {
        char buf [80];
        int rc = read (self->fd, buf, sizeof buf);
        while (rc > 0) {
            printf ("%d bytes read\n", rc);
            rc = read (self->fd, buf, sizeof buf);
        }
        if (rc == 0) {
            printf ("connection closed\n");
            msg_t *msg = msg_new (ZKERNEL_SESSION_CLOSED);
            assert (msg);
            msg->ptr = self;
            mailbox_enqueue (self->owner, msg);
            return 0;
        }
        else {
            assert (errno == EAGAIN);
            return 1;
        }
    }
    else
        return 1;
}

struct io_handler
tcp_session_io_handler (tcp_session_t *self)
{
    static struct io_handler_ops ops = {
        .init  = io_init,
        .event = io_event
    };
    return (struct io_handler) { .object = self, .ops = &ops };
}
