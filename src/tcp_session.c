//  TCP session class

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include "tcp_session.h"
#include "msg.h"
#include "zkernel.h"
#include "event_handler.h"
#include "msg_decoder.h"

struct tcp_session {
    event_handler_t base;
    int fd;
    iobuf_t *iobuf;
    msg_decoder_t *msg_decoder;
    uint8_t *buffer;
    size_t buffer_size;
    int event_mask;
    mailbox_t *owner;
};

tcp_session_t *
tcp_session_new (int fd, msg_decoder_constructor_t *msg_decoder_constructor, mailbox_t *owner)
{
    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self) {
        uint8_t *buffer = malloc (4096);
        size_t buffer_size = 4096;
        *self = (tcp_session_t) {
            .fd = fd,
            .iobuf = iobuf_new (buffer, buffer_size),
            .msg_decoder = msg_decoder_constructor (),
            .buffer = buffer,
            .buffer_size = buffer_size,
            .event_mask = 3,
            .owner = owner
        };
        if (self->iobuf == NULL || self->msg_decoder == NULL || self->buffer == NULL) {
            if (self->iobuf)
                iobuf_destroy (&self->iobuf);
            if (self->msg_decoder)
                msg_decoder_destroy (&self->msg_decoder);
            if (self->buffer)
                free (self->buffer);
            free (self);
            self = NULL;
        }
    }
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
        if (self->iobuf)
            iobuf_destroy (&self->iobuf);
        if (self->msg_decoder)
            msg_decoder_destroy (&self->msg_decoder);
        if (self->buffer)
            free (self->buffer);
        free (self);
        *self_p = NULL;
    }
}

static void
s_send_session_closed (tcp_session_t *self)
{
    session_closed_ev_t *ev = session_closed_ev_new ();
    assert (ev);
    ev->ptr = self;
    mailbox_enqueue (self->owner, (msg_t *) ev);
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
        s_send_session_closed (self);
        *fd = -1;
        self->event_mask = 0;
        return 0;
    }

    if ((flags & ZKERNEL_INPUT_READY) == ZKERNEL_INPUT_READY) {
        msg_decoder_t *decoder = self->msg_decoder;
        iobuf_t *iobuf = self->iobuf;
        int rc = read (self->fd, iobuf->base, iobuf_space (iobuf));
        while (rc > 0) {
            iobuf_put (iobuf, (size_t) rc);
            msg_decoder_result_t res;
            while (iobuf_available (iobuf) > 0) {
                const int rc = msg_decoder_decode (decoder, iobuf, &res);
                // TODO: Handle errors
                assert (rc == 0);
                // TODO: Send frames to session owner
                if (res.frame)
                    frame_destroy (&res.frame);
            }
            if (res.dba_size >= 128)
                msg_decoder_buffer (decoder, iobuf);
            else
                iobuf_init (iobuf, self->buffer, self->buffer_size);

            rc = read (self->fd, iobuf->base, iobuf_space (iobuf));
        }
        if (rc == 0) {
            printf ("tcp_session: Connection closed\n");
            s_send_session_closed (self);
            *fd = -1;
            self->event_mask &= ~1;
        }
        else
            assert (errno == EINTR || errno == EAGAIN);
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
