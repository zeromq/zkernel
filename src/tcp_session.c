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
#include "codec.h"

struct tcp_session {
    io_object_t base;
    int fd;
    frame_t *queue_head;
    frame_t *queue_tail;
    codec_t *codec;
    uint32_t codec_status;
    iobuf_t *sendbuf;
    iobuf_t *recvbuf;
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
    s_input (tcp_session_t *self);

static int
    s_output (tcp_session_t *self);

static struct io_object_ops ops = {
    .init  = io_init,
    .event = io_event,
    .message = s_message,
};

tcp_session_t *
tcp_session_new (int fd, codec_t *codec, mailbox_t *owner)
{
    const size_t min_buffer_size = 64;

    tcp_session_t *self = (tcp_session_t *) malloc (sizeof *self);
    if (self) {
        const size_t buffer_size =
            min_buffer_size > 4096 ? min_buffer_size : 4096;
        *self = (tcp_session_t) {
            .base.ops = ops,
            .fd = fd,
            .codec = codec,
            .sendbuf = iobuf_new (buffer_size),
            .recvbuf = iobuf_new (buffer_size),
            .owner = owner
        };
        if (codec_init (codec, &self->codec_status) == -1)
            goto error;
        if (self->sendbuf == NULL || self->recvbuf == NULL)
            goto error;
    }
    return self;

error:
    close (self->fd);
    if (self->codec)
        codec_destroy (&self->codec);
    if (self->sendbuf)
        iobuf_destroy (&self->sendbuf);
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
        close (self->fd);
        codec_destroy (&self->codec);
        iobuf_destroy (&self->sendbuf);
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
    return ZKERNEL_POLLIN | ZKERNEL_POLLOUT;
}

static int
io_event (io_object_t *self_, uint32_t io_flags, int *fd, uint32_t *timer_interval)
{
    tcp_session_t *self = (tcp_session_t *) self_;
    assert (self);

    codec_t *codec = self->codec;

    if ((io_flags & ZKERNEL_IO_ERROR) != 0)
        goto error;

    uint32_t mask = self->codec_status;
    while ((self->codec_status & mask) != 0) {
        if ((io_flags & ZKERNEL_INPUT_READY) != 0) {
            if (s_input (self) == -1)
                goto error;
            if ((self->codec_status & ZKERNEL_CODEC_WRITE_OK) != 0) {
                io_flags &= ~ZKERNEL_INPUT_READY;
                mask &= ~ZKERNEL_CODEC_WRITE_OK;
            }
        }

        while ((self->codec_status & ZKERNEL_CODEC_DECODER_READY) != 0) {
            frame_t *frame = codec_decode (codec, &self->codec_status);
            if (frame == NULL)
                goto error;
            frame->io_object = self_;
            mailbox_enqueue (self->owner, (msg_t *) frame);
        }

        while (self->queue_head && (self->codec_status & ZKERNEL_CODEC_ENCODER_READY) != 0) {
            frame_t *frame = self->queue_head;
            self->queue_head = (frame_t *) frame->base.next;
            if (self->queue_head == NULL)
                self->queue_tail = NULL;
            if (codec_encode (codec, frame, &self->codec_status) == -1)
                goto error;
        }

        if ((io_flags & ZKERNEL_OUTPUT_READY) != 0) {
            if (s_output (self) == -1)
                goto error;
            if ((self->codec_status & ZKERNEL_CODEC_READ_OK) != 0) {
                io_flags &= ~ZKERNEL_OUTPUT_READY;
                mask &= ~ZKERNEL_CODEC_READ_OK;
            }
        }
    }

    if ((self->codec_status & ZKERNEL_CODEC_READ_OK) != 0 || iobuf_available (self->sendbuf))
        return ZKERNEL_POLLIN | ZKERNEL_POLLOUT;
    else
        return ZKERNEL_POLLIN;

error:
    s_send_session_closed (self);
    *fd = -1;
    return -1;
}

static int
s_input (tcp_session_t *self)
{
    codec_t *codec = self->codec;
    iobuf_t *recvbuf = self->recvbuf;

    while (iobuf_available (recvbuf))
        if (codec_write (codec, recvbuf, &self->codec_status) != 0)
            return -1;

    while ((self->codec_status & ZKERNEL_CODEC_WRITE_OK) != 0) {
        void *buffer;
        size_t buffer_size;
        if (codec_write_buffer (codec, &buffer, &buffer_size) == -1)
            return -1;
        if (buffer_size > 256) {
            const ssize_t rc = recv (self->fd, buffer, buffer_size, 0);
            if (rc == 0)
                return -1;
            if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    return 0;
                else
                    return -1;
            }
            if (codec_write_advance (
                    codec, (size_t) rc, &self->codec_status) != 0)
                return -1;
        }
        else {
            iobuf_reset (recvbuf);
            const ssize_t rc = iobuf_recv (recvbuf, self->fd);
            if (rc == 0)
                return -1;
            if (rc == -1) {
                if (errno == EINTR || errno == EAGAIN)
                    return 0;
                else
                    return -1;
            }
            while (iobuf_available (recvbuf))
                if (codec_write (codec, recvbuf, &self->codec_status) != 0)
                    return -1;
        }
    }

    return 0;
}

static int
s_output (tcp_session_t *self)
{
    codec_t *codec = self->codec;
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

    while ((self->codec_status & ZKERNEL_CODEC_READ_OK) != 0) {
        const void *buffer;
        size_t buffer_size;
        if (codec_read_buffer (codec, &buffer, &buffer_size) == -1)
            return -1;
        if (buffer_size > 256) {
            assert (buffer);
            const ssize_t rc = send (self->fd, buffer, buffer_size, 0);
            if (rc == -1) {
                if (errno == EAGAIN || errno == EINTR)
                    return 0;
                else
                    return -1;
            }
            if (codec_read_advance (
                    codec, (size_t) rc, &self->codec_status) != 0)
                return -1;
        }
        else {
            iobuf_reset (sendbuf);
            const int rc = codec_read (codec, sendbuf, &self->codec_status);
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

    return ZKERNEL_POLLIN | ZKERNEL_POLLOUT;
}

int
tcp_session_send (tcp_session_t *self, const char *data, size_t size)
{
    const int rc = write (self->fd, data, size);
    if (rc == -1)
        return -1;
    return 0;
}
