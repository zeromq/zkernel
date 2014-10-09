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
#include "event_handler.h"
#include "msg_decoder.h"

struct tcp_session {
    event_handler_t base;
    int fd;
    msg_decoder_t *msg_decoder;
    int event_mask;
    mailbox_t *owner;
};

tcp_session_t *
tcp_session_new (int fd, msg_decoder_constructor_t *msg_decoder_constructor, mailbox_t *owner)
{
    msg_decoder_t *msg_decoder = msg_decoder_constructor ();
    if (!msg_decoder)
        return NULL;
    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self)
        *self = (tcp_session_t) { .fd = fd, .msg_decoder = msg_decoder, .event_mask = 3, .owner = owner };
    else
        msg_decoder_destroy (&msg_decoder);
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
    return self->event_mask;
}

static int
io_event (void *self_, uint32_t flags, int *fd, uint32_t *timer_interval)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    if ((flags & ZKERNEL_IO_ERROR) == ZKERNEL_IO_ERROR) {
        printf ("tcp_session: I/O error\n");
        *fd = -1;
        self->event_mask = 0;
        return 0;
    }

    if ((flags & ZKERNEL_INPUT_READY) == ZKERNEL_INPUT_READY) {
        msg_decoder_t *decoder = self->msg_decoder;
        void *buf;
        size_t bufsize;
        msg_decoder_buffer (decoder, &buf, &bufsize);
        int rc = read (self->fd, buf, bufsize);
        while (rc > 0 || (rc == -1 && errno == EINTR)) {
            msg_decoder_data_ready (decoder, (size_t) rc);
            while ((rc = msg_decoder_decode (decoder)) > 0) {
                printf ("decoded %d messages\n", rc);
            }
            // XXX Handle message decoding errors
            assert (rc != -1);
            msg_decoder_buffer (decoder, &buf, &bufsize);
            rc = read (self->fd, buf, sizeof buf);
        }
        if (rc == 0) {
            printf ("tcp_session: Connection closed\n");
            s_send_session_closed (self);
            *fd = -1;
            self->event_mask &= ~1;
        }
        else
            assert (errno == EAGAIN);
    }
    if ((flags & ZKERNEL_OUTPUT_READY) == ZKERNEL_OUTPUT_READY) {
        ready_to_send_ev_t *ev = ready_to_send_ev_new ();
        assert (ev);
        ev->ptr = self;
        mailbox_enqueue (self->owner, (msg_t *) ev);
        self->event_mask &= ~2;
        return self->event_mask;
    }
    return self->event_mask;
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

int
tcp_session_send (tcp_session_t *self, const char *data, size_t size)
{
    const int rc = write (self->fd, data, size);
    if (rc == -1)
        return -1;
    return 0;
}
