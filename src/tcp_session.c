//  TCP session class

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "tcp_session.h"

struct tcp_session {
    int fd;
};

tcp_session_t *
tcp_session_new (int fd)
{
    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self)
        *self = (tcp_session_t) { .fd = fd };
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
io_event (void *self_, uint32_t flags)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    if ((flags & 1) == 1) {
        char buf [80];
        int rc = read (self->fd, buf, sizeof buf);
        while (rc > 0) {
            printf ("%d bytes read\n", rc);
            rc = read (self->fd, buf, sizeof buf);
        }
        if (rc == 0) {
            printf ("connection closed\n");
            return 0;
        }
        else {
            assert (errno == EAGAIN);
            return 1;
        }
    }
    else
        return 1 | 2;
}

static void
io_error (void *self_)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);
    printf ("I/O error\n");
}

struct io_handler
tcp_session_io_handler (tcp_session_t *self)
{
    static struct io_handler_ops ops = {
        .event = io_event,
        .error = io_error
    };
    return (struct io_handler) { .object = self, .ops = &ops };
}

int
tcp_session_fd (tcp_session_t *self)
{
    assert (self);
    return self->fd;
}
