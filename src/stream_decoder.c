/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "stream_decoder.h"
#include "frame.h"

struct stream_decoder {
    decoder_t base;
    frame_t *frame;
};

typedef struct stream_decoder stream_decoder_t;

static struct decoder_ops decoder_ops;

static stream_decoder_t *
s_new ()
{
    stream_decoder_t *self = malloc (sizeof *self);
    if (self) {
        *self = (stream_decoder_t) {
            .base.ops = decoder_ops,
            .frame = frame_new ()
        };
        if (self->frame == NULL) {
            free (self);
            self = NULL;
        }
    }
    return self;
}

static int
s_write (decoder_t *self_, iobuf_t *iobuf, decoder_status_t *status)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;

    frame->frame_size += iobuf_read (iobuf,
        frame->frame_data, sizeof frame->frame_data - frame->frame_size);

    const size_t free_space =
        sizeof frame->frame_data - frame->frame_size;
    *status = free_space & DECODER_BUFFER_MASK;
    if (frame->frame_size > 0)
        *status |= DECODER_READY;
    if (free_space > 0)
        *status |= DECODER_WRITE_OK;

    return 0;
}

static int
s_buffer (decoder_t *self_, void **buffer, size_t *buffer_size)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;

    *buffer = frame->frame_data + frame->frame_size;
    *buffer_size = sizeof frame->frame_data - frame->frame_size;

    return 0;
}

static int
s_advance (decoder_t *self_, size_t n, decoder_status_t *status)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;
    if (frame->frame_size + n > sizeof frame->frame_data)
        return -1;

    frame->frame_size += n;

    const size_t free_space =
        sizeof frame->frame_data - frame->frame_size;
    *status = free_space & DECODER_BUFFER_MASK;
    if (frame->frame_size > 0)
        *status |= DECODER_READY;
    if (free_space > 0)
        *status |= DECODER_WRITE_OK;

    return 0;
}

static frame_t *
s_decode (decoder_t *self_, decoder_status_t *status)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self != NULL);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return NULL;

    self->frame = frame_new ();
    if (self->frame)
        *status = (sizeof frame->frame_data & DECODER_BUFFER_MASK)
                | DECODER_WRITE_OK;
    else
        *status = DECODER_ERROR;

    return frame;
}

static decoder_status_t
s_status (decoder_t *self_)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    const frame_t *frame = self->frame;
    if (frame == NULL)
        return DECODER_ERROR;
    else {
        const size_t free_space =
            sizeof frame->frame_data - frame->frame_size;
        decoder_status_t status =
            free_space & DECODER_BUFFER_MASK;
        if (frame->frame_size > 0)
            status |= DECODER_READY;
        if (free_space > 0 )
            status |= DECODER_WRITE_OK;
        return status;
    }
}

static void
s_destroy (decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        stream_decoder_t *self = (stream_decoder_t *) *self_p;
        free (self);
        *self_p = NULL;
    }
}

static struct decoder_ops decoder_ops = {
    .write = s_write,
    .buffer = s_buffer,
    .advance = s_advance,
    .decode = s_decode,
    .status = s_status,
    .destroy = s_destroy
};

decoder_t *
stream_decoder_create_decoder ()
{
    return (decoder_t *) s_new ();
}
