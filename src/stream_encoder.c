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

static int
    s_encode (encoder_t *base, frame_t *frame);

static int
    s_read (encoder_t *base, iobuf_t *iobuf);

static uint8_t *
    s_buffer (encoder_t *base);

static void
    s_destroy (encoder_t **base_p);

static stream_encoder_t *
s_new ()
{
    static struct encoder_ops ops = {
        .encode = s_encode,
        .read = s_read,
        .buffer = s_buffer,
        .destroy = s_destroy
    };

    stream_encoder_t *self = malloc (sizeof *self);
    if (self)
        *self = (stream_encoder_t) {
            .base = (encoder_t) { .ready = true, .ops = ops }
        };
    return self;
}

static int
s_encode (encoder_t *base, frame_t *frame)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    if (self->frame) {
        assert (self->bytes_left == 0);
        frame_destroy (&self->frame);
    }

    self->base.ready = frame->frame_size == 0;
    self->base.has_data = frame->frame_size > 0;
    self->base.dba_size = frame->frame_size;
    self->frame = frame;
    self->ptr = frame->frame_data;
    self->bytes_left = frame->frame_size;

    return 0;
}

static int
s_read (encoder_t *base, iobuf_t *iobuf)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    const size_t n =
        iobuf_write (iobuf, self->ptr, self->bytes_left);
    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left > 0)
        self->base = (encoder_t) {
            .has_data = true,
            .dba_size = self->bytes_left };
    else {
        frame_destroy (&self->frame);
        self->base = (encoder_t) { .ready = true };
    }

    return 0;
}

static uint8_t *
s_buffer (encoder_t *base)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    return self->frame ? self->ptr : NULL;
}

static int
s_advance (void *base, size_t n)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    assert (n <= self->bytes_left);
    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left > 0)
        self->base = (encoder_t) {
            .has_data = true,
            .dba_size = self->bytes_left };
    else {
        frame_destroy (&self->frame);
        self->base = (encoder_t) { .ready = true };
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
