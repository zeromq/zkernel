#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "null_decoder.h"
#include "frame.h"

struct null_decoder {
    frame_t *frame;
};

typedef struct null_decoder null_decoder_t;

static null_decoder_t *
s_new ()
{
    null_decoder_t *self = malloc (sizeof *self);
    if (self) {
        *self = (null_decoder_t) { .frame = frame_new () };
        if (self->frame == NULL) {
            free (self);
            self = NULL;
        }
    }
    return self;
}

static int
s_write (void *self_, iobuf_t *iobuf, msg_decoder_info_t *info)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;

    frame->frame_size += iobuf_read (iobuf,
        frame->frame_data, sizeof frame->frame_data - frame->frame_size);

    *info = (msg_decoder_info_t) { .ready = frame->frame_size > 0 };
    return 0;
}

static uint8_t *
s_buffer (void *self_)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    return frame ? frame->frame_data + frame->frame_size : NULL;
}

static int
s_advance (void *self_, size_t n, msg_decoder_info_t *info)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return -1;

    assert (frame->frame_size + n <= sizeof frame->frame_data);
    frame->frame_size += n;
    *info = (msg_decoder_info_t) { .ready = frame->frame_size > 0 };
    return 0;
}

static frame_t *
s_decode (void *self_, msg_decoder_info_t *info)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self != NULL);

    frame_t *frame = self->frame;
    if (frame == NULL)
        return NULL;

    self->frame = frame_new ();
    if (self->frame)
        *info = (msg_decoder_info_t) {
            .dba_size = sizeof self->frame->frame_data };
    else
        *info = (msg_decoder_info_t) {};

    return frame;
}

static void
s_destroy (void **self_p)
{
    assert (self_p);
    if (*self_p) {
        null_decoder_t *self = (null_decoder_t *) *self_p;
        free (self);
        *self_p = NULL;
    }
}

msg_decoder_t *
null_decoder_create_decoder ()
{
    static struct msg_decoder_ops ops = {
        .write = s_write,
        .buffer = s_buffer,
        .advance = s_advance,
        .decode = s_decode,
        .destroy = s_destroy
    };

    msg_decoder_t *self = (msg_decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (msg_decoder_t) { .object = s_new (), .ops = ops };
        if (self->object == NULL) {
            free (self);
            self = NULL;
        }
    }

    return self;
}
