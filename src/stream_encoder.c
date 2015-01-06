/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "frame.h"
#include "encoder.h"

struct stream_encoder {
    encoder_t base;
    uint8_t *ptr;
    size_t bytes_left;
    frame_t *frame;
};

typedef struct stream_encoder stream_encoder_t;

static void
    s_info (encoder_t *base, encoder_info_t *info);

static int
    s_encode (encoder_t *base, frame_t *frame, encoder_info_t *info);

static int
    s_read (encoder_t *base, iobuf_t *iobuf, encoder_info_t *info);

static const void *
    s_buffer (encoder_t *base);

static int
    s_advance (encoder_t *base, size_t n, encoder_info_t *info);

static void
    s_destroy (encoder_t **base_p);

static stream_encoder_t *
s_new ()
{
    static struct encoder_ops ops = {
        .info = s_info,
        .encode = s_encode,
        .read = s_read,
        .buffer = s_buffer,
        .advance = s_advance,
        .destroy = s_destroy
    };

    stream_encoder_t *self = malloc (sizeof *self);
    if (self)
        *self = (stream_encoder_t) {
            .base = (encoder_t) { .ops = ops }
        };
    return self;
}

static void
s_info (encoder_t *base, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    *info = (encoder_info_t) {
        .ready = self->bytes_left == 0,
        .dba_size = self->bytes_left
    };
}

static int
s_encode (encoder_t *base, frame_t *frame, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    if (self->frame) {
        assert (self->bytes_left == 0);
        frame_destroy (&self->frame);
    }

    self->frame = frame;
    self->ptr = frame->frame_data;
    self->bytes_left = frame->frame_size;

    *info = (encoder_info_t) {
        .ready = frame->frame_size == 0,
        .dba_size = frame->frame_size
    };

    return 0;
}

static int
s_read (encoder_t *base, iobuf_t *iobuf, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    const size_t n =
        iobuf_write (iobuf, self->ptr, self->bytes_left);
    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left > 0)
        *info = (encoder_info_t) { .dba_size = self->bytes_left };
    else {
        frame_destroy (&self->frame);
        *info = (encoder_info_t) { .ready = true };
    }

    return 0;
}

static const void *
s_buffer (encoder_t *base)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    return self->frame ? self->ptr : NULL;
}

static int
s_advance (encoder_t *base, size_t n, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    assert (n <= self->bytes_left);
    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left > 0)
        *info = (encoder_info_t) { .dba_size = self->bytes_left };
    else {
        frame_destroy (&self->frame);
        *info = (encoder_info_t) { .ready = true };
    }

    return 0;
}

static void
s_destroy (encoder_t **base_p)
{
    if (*base_p) {
        stream_encoder_t *self = (stream_encoder_t *) *base_p;
        if (self->frame)
            frame_destroy (&self->frame);
        free (self);
        *base_p = NULL;
    }
}

encoder_t *
stream_encoder_create_encoder ()
{
    return (encoder_t *) s_new ();
}
