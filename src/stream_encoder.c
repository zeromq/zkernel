#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "frame.h"
#include "encoder.h"

struct stream_encoder {
    uint8_t *ptr;
    size_t bytes_left;
    frame_t *frame;
};

typedef struct stream_encoder stream_encoder_t;

static stream_encoder_t *
s_new ()
{
    stream_encoder_t *self = malloc (sizeof *self);
    if (self)
        *self = (stream_encoder_t) {};
    return self;
}

static int
s_encode (void *self_, frame_t *frame, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) self_;
    assert (self);

    if (self->frame) {
        assert (self->bytes_left == 0);
        frame_destroy (&self->frame);
    }

    self->frame = frame;
    self->ptr = frame->frame_data;
    self->bytes_left = frame->frame_size;

    *info = (encoder_info_t) { .dba_size = self->bytes_left };
    return 0;
}

static int
s_read (void *self_, iobuf_t *iobuf, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) self_;
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

static uint8_t *
s_buffer (void *self_)
{
    stream_encoder_t *self = (stream_encoder_t *) self_;
    assert (self);

    return self->frame ? self->ptr : NULL;
}

static int
s_advance (void *self_, size_t n, encoder_info_t *info)
{
    stream_encoder_t *self = (stream_encoder_t *) self_;
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
s_destroy (void **self_p)
{
    if (*self_p) {
        stream_encoder_t *self = (stream_encoder_t *) *self_p;
        if (self->frame)
            frame_destroy (&self->frame);
        free (self);
        *self_p = NULL;
    }
}

encoder_t *
stream_encoder_create_encoder ()
{
    static struct encoder_ops ops = {
        .encode = s_encode,
        .read = s_read,
        .buffer = s_buffer,
        .destroy = s_destroy
    };

    encoder_t *self = (encoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (encoder_t) { .object = s_new (), .ops = ops };
        if (self->object == NULL) {
            free (self);
            self = NULL;
        }
    }

    return self;
}
