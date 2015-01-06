//  TCP session class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "io_object.h"
#include "tcp_session.h"
#include "msg.h"
#include "zkernel.h"
#include "protocol.h"
#include "encoder.h"
#include "decoder.h"

struct tcp_session {
    io_object_t base;
    int fd;
    frame_t *queue_head;
    frame_t *queue_tail;
    protocol_t *protocol;
    encoder_t *encoder;
    encoder_info_t encoder_info;
    iobuf_t *sendbuf;
    decoder_t *decoder;
    decoder_info_t decoder_info;
    iobuf_t *recvbuf;
    int event_mask;
    mailbox_t *owner;
};

static int
    io_init (io_object_t *self_, int *fd, uint32_t *timer_interval);

static int
    s_handshake (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval);

static int
    io_event (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval);

static int
    s_message (io_object_t *self_, msg_t *msg);

static int
    s_encode (tcp_session_t *self);

static int
    s_decode (tcp_session_t *self);

static struct io_object_ops ops = {
    .init  = io_init,
    .event = s_handshake,
    .message = s_message,
};

tcp_session_t *
tcp_session_new (int fd, protocol_t *protocol, mailbox_t *owner)
{
    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self) {
        const size_t min_buffer_size = protocol_min_buffer_size (protocol);
        const size_t buffer_size =
            min_buffer_size > 4096 ? min_buffer_size : 4096;
        *self = (tcp_session_t) {
            .base.ops = ops,
            .fd = fd,
            .protocol = protocol,
            .sendbuf = iobuf_new (buffer_size),
            .recvbuf = iobuf_new (buffer_size),
            .event_mask = ZKERNEL_POLLIN | ZKERNEL_POLLOUT,
            .owner = owner
        };

        if (self->sendbuf == NULL || self->recvbuf == NULL)
            goto error;

        if (protocol_is_handshake_complete (protocol)) {
            self->encoder = protocol_encoder (protocol, &self->encoder_info);
            if (self->encoder == NULL)
                goto error;
            self->decoder = protocol_decoder (protocol, &self->decoder_info);
            if (self->decoder == NULL)
                goto error;
        }
    }
    return self;

error:
    if (self->encoder)
        encoder_destroy (&self->encoder);
    if (self->sendbuf)
        iobuf_destroy (&self->sendbuf);
    if (self->decoder)
        decoder_destroy (&self->decoder);
    if (self->recvbuf)
        iobuf_destroy (&self->recvbuf);
    free (self);

    return NULL;
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
        if (self->encoder)
            encoder_destroy (&self->encoder);
        if (self->sendbuf)
            iobuf_destroy (&self->sendbuf);
        if (self->decoder)
            decoder_destroy (&self->decoder);
        if (self->recvbuf)
            iobuf_destroy (&self->recvbuf);
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
s_handshake (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    protocol_t *protocol = self->protocol;

    if ((flags & ZKERNEL_IO_ERROR) == ZKERNEL_IO_ERROR)
        goto error;

    if ((flags & ZKERNEL_INPUT_READY) == ZKERNEL_INPUT_READY) {
        if (iobuf_available (self->recvbuf) == 0)
            iobuf_reset (self->recvbuf);

        if (iobuf_space (self->recvbuf) > 0) {
            const ssize_t rc = iobuf_recv (self->recvbuf, self->fd);
            if (rc == 0)
                goto error;
            if (rc == -1 && (errno != EAGAIN && errno != EINTR))
                goto error;
        }
    }

    const int rc = protocol_handshake (
        protocol, self->recvbuf, self->sendbuf);
    if (rc == -1)
        goto error;

    if ((flags & ZKERNEL_OUTPUT_READY) == ZKERNEL_OUTPUT_READY) {
        const ssize_t rc = iobuf_send (self->sendbuf, self->fd);
        if (rc == -1)
            goto error;

        if (iobuf_available (self->sendbuf) == 0)
            iobuf_reset (self->sendbuf);
    }

    if (iobuf_space (self->recvbuf) == 0)
        self->event_mask &= ~ZKERNEL_POLLIN;

    if (iobuf_available (self->sendbuf) == 0)
        self->event_mask &= ~ZKERNEL_POLLOUT;

    if (protocol_is_handshake_complete (protocol)) {
        self->encoder = protocol_encoder (protocol, &self->encoder_info);
        if (self->encoder == NULL)
            goto error;
        self->decoder = protocol_decoder (protocol, &self->decoder_info);
        if (self->decoder == NULL)
            goto error;
        self->base.ops.event = io_event;
        self->event_mask = ZKERNEL_POLLIN | ZKERNEL_POLLOUT;
    }

    return self->event_mask;

error:
    printf ("tcp_session: closed\n");
    s_send_session_closed (self);
    *fd = -1;
    self->event_mask = 0;
    return 0;
}

static int
io_event (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    if ((flags & ZKERNEL_IO_ERROR) == ZKERNEL_IO_ERROR)
        goto error;
    if ((flags & ZKERNEL_INPUT_READY) == ZKERNEL_INPUT_READY) {
        const int rc = s_decode (self);
        if (rc == -1)
            goto error;
    }
    if ((flags & ZKERNEL_OUTPUT_READY) == ZKERNEL_OUTPUT_READY) {
        encoder_info_t *info = &self->encoder_info;
        const int rc = s_encode (self);
        if (rc == -1)
            goto error;
        if (info->dba_size == 0 || iobuf_available (self->sendbuf) == 0)
            self->event_mask &= ~ZKERNEL_POLLOUT;
    }
    return self->event_mask;

error:
    printf ("tcp_session: closed\n");
    s_send_session_closed (self);
    *fd = -1;
    self->event_mask = 0;
    return 0;
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
        self->event_mask |= ZKERNEL_POLLOUT;
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
s_encode (tcp_session_t *self)
{
    encoder_t *encoder = self->encoder;
    encoder_info_t *info = &self->encoder_info;
    iobuf_t *sendbuf = self->sendbuf;

    while (iobuf_available (sendbuf)) {
        const ssize_t rc = iobuf_send (sendbuf, self->fd);
        if (rc == -1) {
            if (errno == EAGAIN || errno == EINTR)
                return 0;
            else
                return -1;
        }
    }

    while (info->dba_size > 0
        || iobuf_available (sendbuf) > 0
        || info->ready && self->queue_head)
    {
        while (info->ready && self->queue_head) {
            frame_t *frame = self->queue_head;
            self->queue_head = (frame_t *) frame->base.next;
            if (self->queue_head == NULL)
                self->queue_tail = NULL;
            if (encoder_encode (encoder, frame, info) != 0)
                return -1;
        }
        if (info->dba_size >= 256) {
            const void *buffer = encoder_buffer (encoder);
            assert (buffer);
            const ssize_t rc = send (self->fd, buffer, info->dba_size, 0);
            if (rc == -1) {
                if (errno == EAGAIN || errno == EINTR)
                    return 0;
                else
                    return -1;
            }
            if (encoder_advance (encoder, (size_t) rc, info) != 0)
                return -1;
        }
        else {
            iobuf_reset (sendbuf);
            const int rc = encoder_read (encoder, sendbuf, info);
            if (rc == -1)
                return -1;
            while (iobuf_available (sendbuf)) {
                const ssize_t rc = iobuf_send (sendbuf, self->fd);
                if (rc == -1) {
                    if (errno == EAGAIN || errno == EINTR)
                        return 0;
                    else
                        return -1;
                }
            }
        }
    }

    return 0;
}

static int
s_decode (tcp_session_t *self)
{
    decoder_t *decoder = self->decoder;
    decoder_info_t *info = &self->decoder_info;
    iobuf_t *recvbuf = self->recvbuf;

    while (1) {
        while (info->ready) {
            frame_t *frame = decoder_decode (decoder, info);
            if (frame == NULL)
                goto error;
            frame->io_object = (io_object_t *) self;
            mailbox_enqueue (self->owner, (msg_t *) frame);
        }

        if (iobuf_available (recvbuf)) {
            if (decoder_write (decoder, recvbuf, info) != 0)
                goto error;
        }
        else
        if (info->dba_size >= 256) {
            void *buffer = decoder_buffer (decoder);
            assert (buffer);
            const ssize_t rc = recv (self->fd, buffer, info->dba_size, 0);
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
        }
        else {
            iobuf_reset (recvbuf);
            const ssize_t rc = iobuf_recv (recvbuf, self->fd);
            if (rc == 0)
                goto error;
            if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    break;
                else
                    goto error;
            }
            if (decoder_write (decoder, recvbuf, info) != 0)
                goto error;
        }
    }
    return 0;

error:
    return -1;
}
