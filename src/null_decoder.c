#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "null_decoder.h"
#include "frame.h"

struct null_decoder {
    int error;
};

typedef struct null_decoder null_decoder_t;

static null_decoder_t *
s_new ()
{
    null_decoder_t *self = malloc (sizeof *self);
    if (self)
        *self = (null_decoder_t) {};
    return self;
}

static int
s_buffer (void *self, iobuf_t *iobuf)
{
    return -1;
}

static int
s_decode (void *self_, iobuf_t *iobuf, msg_decoder_result_t *res)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self != NULL);

    if (iobuf_available (iobuf) == 0) {
        *res = (msg_decoder_result_t) {};
        return 0;
    }
    frame_t *frame = frame_new ();
    if (frame == NULL) {
        // TODO: Set error value
        *res = (msg_decoder_result_t) {};
        return -1;
    }
    frame->frame_size = iobuf_read (
        iobuf, frame->frame_data, sizeof frame->frame_data);

    *res = (msg_decoder_result_t) { .frame = frame };
    return 0;
}

static int
s_error (void *self_)
{
    return 0;
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
        .buffer = s_buffer,
        .decode = s_decode,
        .error = s_error,
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
