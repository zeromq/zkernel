#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream_decoder.h"
#include "frame.h"

struct stream_decoder {
    frame_t *frame;
};

typedef struct stream_decoder stream_decoder_t;

static stream_decoder_t *
s_new ()
{
    stream_decoder_t *self = malloc (sizeof *self);
    if (self) {
        *self = (stream_decoder_t) { .frame = frame_new () };
        if (self->frame == NULL) {
            free (self);
            self = NULL;
        }
    }
    return self;
}

static void
s_info (void *self_, decoder_info_t *info)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame)
        *info = (decoder_info_t) {
            .ready = frame->frame_size > 0,
            .dba_size = sizeof frame->frame_data - frame->frame_size
        };
    else
        *info = (decoder_info_t) { .ready = false, .dba_size = 0 };
}

static int
s_write (void *self_, iobuf_t *iobuf, decoder_info_t *info)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;

    frame->frame_size += iobuf_read (iobuf,
        frame->frame_data, sizeof frame->frame_data - frame->frame_size);

    *info = (decoder_info_t) {
        .ready = frame->frame_size > 0,
        .dba_size = sizeof frame->frame_data - frame->frame_size
    };

    return 0;
}

static void *
s_buffer (void *self_)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    return frame ? frame->frame_data + frame->frame_size : NULL;
}

static int
s_advance (void *self_, size_t n, decoder_info_t *info)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;

    assert (frame->frame_size + n <= sizeof frame->frame_data);
    frame->frame_size += n;
    *info = (decoder_info_t) { .ready = frame->frame_size > 0 };
    return 0;
}

static frame_t *
s_decode (void *self_, decoder_info_t *info)
{
    stream_decoder_t *self = (stream_decoder_t *) self_;
    assert (self != NULL);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return NULL;

    self->frame = frame_new ();
    if (self->frame)
        *info = (decoder_info_t) {
            .dba_size = sizeof self->frame->frame_data };
    else
        *info = (decoder_info_t) {};

    return frame;
}

static void
s_destroy (void **self_p)
{
    assert (self_p);
    if (*self_p) {
        stream_decoder_t *self = (stream_decoder_t *) *self_p;
        free (self);
        *self_p = NULL;
    }
}

decoder_t *
stream_decoder_create_decoder ()
{
    static struct decoder_ops ops = {
        .info = s_info,
        .write = s_write,
        .buffer = s_buffer,
        .advance = s_advance,
        .decode = s_decode,
        .destroy = s_destroy
    };

    decoder_t *self = (decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (decoder_t) { .object = s_new (), .ops = ops };
        if (self->object == NULL) {
            free (self);
            self = NULL;
        }
    }

    return self;
}
