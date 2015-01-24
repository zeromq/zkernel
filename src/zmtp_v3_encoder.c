/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "frame.h"
#include "encoder.h"

#define WAITING_FOR_FRAME   0
#define READING_HEADER      1
#define READING_BODY        2

struct zmtp_v3_encoder {
    encoder_t base;
    int state;
    uint8_t buffer [9];
    frame_t *frame;
    uint8_t *ptr;
    size_t bytes_left;
};

typedef struct zmtp_v3_encoder zmtp_v3_encoder_t;

static void
put_uint64 (uint8_t *ptr, uint64_t n);

static struct encoder_ops encoder_ops;

zmtp_v3_encoder_t *
s_new ()
{
    zmtp_v3_encoder_t *self =
        (zmtp_v3_encoder_t *) malloc (sizeof *self);
    if (self)
        *self = (zmtp_v3_encoder_t) {
            .base = (encoder_t) { .ops = encoder_ops },
            .state = WAITING_FOR_FRAME
        };
    return self;
}

static int
s_encode (encoder_t *base, frame_t *frame, uint32_t *status)
{
    zmtp_v3_encoder_t *self = (zmtp_v3_encoder_t *) base;
    assert (self);

    if (self->state != WAITING_FOR_FRAME)
        return -1;

    assert (self->frame == NULL);
    self->frame = frame;

    uint8_t *buffer = self->buffer;
    buffer [0] = 0;     // flags
    if (frame->frame_size > 255) {
        buffer [0] |= 0x02;
        put_uint64 (buffer + 1, frame->frame_size);
        self->bytes_left = 9;
    }
    else {
        buffer [1] = (uint8_t) frame->frame_size;
        self->bytes_left = 2;
    }

    *status = ZKERNEL_ENCODER_READ_OK;
    return 0;
}

static int
s_read (encoder_t *base, iobuf_t *iobuf, uint32_t *status)
{
    zmtp_v3_encoder_t *self = (zmtp_v3_encoder_t *) base;
    assert (self);

    if (self->state == WAITING_FOR_FRAME)
        return -1;

    assert (self->state == READING_HEADER
         || self->state == READING_BODY);

    if (self->state == READING_HEADER) {
        const size_t n =
            iobuf_write (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;

        if (self->bytes_left == 0) {
            assert (self->frame);
            self->ptr = self->frame->frame_data;
            self->bytes_left = self->frame->frame_size;
            self->state = READING_BODY;
        }
    }

    if (self->state == READING_BODY) {
        const size_t n =
            iobuf_write (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;

        if (self->bytes_left == 0) {
            frame_destroy (&self->frame);
            self->state = WAITING_FOR_FRAME;
        }
    }

    *status = self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK;
    if (self->bytes_left)
        *status |= ZKERNEL_ENCODER_READY;
    else
        *status |= ZKERNEL_ENCODER_READ_OK;

    return 0;
}

static int
s_buffer (encoder_t *base, const void **buffer, size_t *buffer_size)
{
    zmtp_v3_encoder_t *self = (zmtp_v3_encoder_t *) base;
    assert (self);

    if (self->state == WAITING_FOR_FRAME) {
        *buffer = NULL;
        *buffer_size = 0;
    }
    else {
        *buffer = self->ptr;
        *buffer_size = self->bytes_left;
    }

    return 0;
}

static int
s_advance (encoder_t *base, size_t n, uint32_t *status)
{
    zmtp_v3_encoder_t *self = (zmtp_v3_encoder_t *) base;
    assert (self);

    if (self->state == WAITING_FOR_FRAME)
        return -1;

    assert (self->state == READING_HEADER
         || self->state == READING_BODY);

    if (n > self->bytes_left)
        return -1;

    self->ptr += n;
    self->bytes_left -= n;

    if (self->state == READING_HEADER) {
        if (self->bytes_left == 0) {
            frame_t *frame = self->frame;
            assert (frame);
            self->ptr = frame->frame_data;
            self->bytes_left = frame->frame_size;
            self->state = READING_BODY;
        }
    }

    if (self->bytes_left == 0) {
        frame_destroy (&self->frame);
        self->state = WAITING_FOR_FRAME;
        *status = ZKERNEL_ENCODER_READY;
    }
    else
        *status = (self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK)
                | ZKERNEL_ENCODER_READ_OK;

    return 0;
}

static uint32_t
s_status (encoder_t *base)
{
    zmtp_v3_encoder_t *self = (zmtp_v3_encoder_t *) base;
    assert (self);

    if (self->state == WAITING_FOR_FRAME)
        return ZKERNEL_ENCODER_READY;
    else
        return (self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK)
              | ZKERNEL_ENCODER_READ_OK;

    return 0;
}

static void
s_destroy (encoder_t **base_p)
{
    if (*base_p) {
        zmtp_v3_encoder_t *self = (zmtp_v3_encoder_t *) *base_p;
        if (self->frame)
            frame_destroy (&self->frame);
        free (self);
        *base_p = NULL;
    }
}

static struct encoder_ops ops = {
    .encode = s_encode,
    .read = s_read,
    .buffer = s_buffer,
    .advance = s_advance,
    .status = s_status,
    .destroy = s_destroy
};

static void
put_uint64 (uint8_t *ptr, uint64_t n)
{
    *ptr++ = (uint8_t) (n >> 56);
    *ptr++ = (uint8_t) (n >> 48);
    *ptr++ = (uint8_t) (n >> 40);
    *ptr++ = (uint8_t) (n >> 32);
    *ptr++ = (uint8_t) (n >> 24);
    *ptr++ = (uint8_t) (n >> 16);
    *ptr++ = (uint8_t) (n >> 8);
    *ptr   = (uint8_t)  n;
}
