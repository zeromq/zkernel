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
#include "decoder.h"

struct tcp_session {
    io_object_t base;
    int fd;
    frame_t *queue_head;
    frame_t *queue_tail;
    iobuf_t *iobuf;
    decoder_t *decoder;
    decoder_info_t info;
    size_t buffer_size;
    int event_mask;
    mailbox_t *owner;
};

static int
    io_init (io_object_t *self_, int *fd, uint32_t *timer_interval);

static int
    io_event (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval);

static int
    s_message (io_object_t *self_, msg_t *msg);

static int
    s_decode (tcp_session_t *self);

static struct io_object_ops ops = {
    .init  = io_init,
    .event = io_event,
    .message = s_message,
};

tcp_session_t *
tcp_session_new (int fd, decoder_constructor_t *decoder_constructor, mailbox_t *owner)
{
    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self) {
        const size_t buffer_size = 4096;
        *self = (tcp_session_t) {
            .base.ops = ops,
            .fd = fd,
            .iobuf = iobuf_new (buffer_size),
            .decoder = decoder_constructor (),
            .buffer_size = buffer_size,
            .event_mask = 3,
            .owner = owner
        };
        if (self->iobuf == NULL || self->decoder == NULL) {
            if (self->iobuf)
                iobuf_destroy (&self->iobuf);
            if (self->decoder)
                decoder_destroy (&self->decoder);
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
        msg_t *msg = (msg_t *) self->queue_head;
        while (msg) {
            msg_t *next = msg->next;
            msg_destroy (&msg);
            msg = next;
        }
        if (self->fd != -1)
            close (self->fd);
        if (self->iobuf)
            iobuf_destroy (&self->iobuf);
        if (self->decoder)
            decoder_destroy (&self->decoder);
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
io_init (io_object_t *self_, int *fd, uint32_t *timer_interval)
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
io_event (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval)
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
        const int rc = s_decode (self);
        if (rc == -1) {
            printf ("tcp_session: Connection closed\n");
            s_send_session_closed (self);
            *fd = -1;
            self->event_mask &= ~1;
        }
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

static int
s_message (io_object_t *self_, msg_t *msg)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    if (msg->msg_type == ZKERNEL_MSG_TYPE_FRAME) {
        frame_t *frame = (frame_t *) msg;
        if (self->queue_head == NULL)
            self->queue_head = self->queue_tail = frame;
        else {
            self->queue_tail->base.next = (msg_t *) frame;
            self->queue_tail = frame;
        }
        frame->base.next = NULL;
    }
    else
        msg_destroy (&msg);

    return self->event_mask;
}

int
tcp_session_send (tcp_session_t *self, const char *data, size_t size)
{
    const int rc = write (self->fd, data, size);
    if (rc == -1)
        return -1;
    return 0;
}

static int
s_decode (tcp_session_t *self)
{
    decoder_t *decoder = self->decoder;
    decoder_info_t *info = &self->info;
    iobuf_t *iobuf = self->iobuf;

    assert (iobuf_available (iobuf) == 0);

    while (1) {
        if (info->dba_size >= 256) {
            uint8_t *buffer = decoder_buffer (decoder);
            assert (buffer);
            const int rc = read (self->fd, buffer, info->dba_size);
            if (rc == 0)
                goto error;
            if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    break;
                else
                    goto error;
            }
            if (decoder_advance (decoder, (size_t) rc, info) != 0)
                goto error;
            while (info->ready) {
                frame_t *frame = decoder_decode (decoder, info);
                if (frame == NULL)
                    goto error;
                frame->io_object = (io_object_t *) self;
                mailbox_enqueue (self->owner, (msg_t *) frame);
            }
        }
        else {
            const ssize_t rc = iobuf_recv (iobuf, self->fd);
            if (rc == 0)
                goto error;
            if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    break;
                else
                    goto error;
            }
            while (info->ready || iobuf_available (iobuf) > 0) {
                if (iobuf_available (iobuf) > 0)
                    if (decoder_write (decoder, iobuf, info) != 0)
                        goto error;
                while (info->ready) {
                    frame_t *frame = decoder_decode (decoder, info);
                    if (frame == NULL)
                        goto error;
                    frame->io_object = (io_object_t *) self;
                    mailbox_enqueue (self->owner, (msg_t *) frame);
                }
            }
            iobuf_reset (iobuf);
        }
    }
    return 0;

error:
    return -1;
}
