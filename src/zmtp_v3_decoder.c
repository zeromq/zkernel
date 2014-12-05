//  ZMTP v3 decoder class

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "frame.h"
#include "decoder.h"

struct zmtp_v3_decoder {
    int state;
    uint8_t buffer [9];
    frame_t *frame;
    uint8_t *ptr;
    size_t bytes_left;
};

typedef struct zmtp_v3_decoder zmtp_v3_decoder_t;

#define DECODING_FLAGS      0
#define DECODING_LENGTH     1
#define DECODING_BODY       2
#define FRAME_READY         3

static size_t
s_decode_length (const uint8_t *ptr);

static zmtp_v3_decoder_t *
s_new ()
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v3_decoder_t) {
            .ptr = self->buffer,
            .bytes_left = 1,
            .state = DECODING_FLAGS
        };
    }
    return self;
}

static int
s_write (void *self_, iobuf_t *iobuf, decoder_info_t *info)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    if (self->state == DECODING_FLAGS) {
        const size_t n =
            iobuf_read (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;
        if (self->bytes_left == 0) {
            if ((self->buffer [0] & 0x02) == 0x02)
                self->bytes_left = 8;
            else
                self->bytes_left = 1;
            self->state = DECODING_LENGTH;
        }
    }

    if (self->state == DECODING_LENGTH) {
        const size_t n =
            iobuf_read (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;
        if (self->bytes_left == 0) {
            const size_t frame_size = (size_t) s_decode_length (self->buffer);
            self->frame = frame_new_with_size (frame_size);
            if (self->frame == NULL)
                return -1;
            self->ptr = self->frame->frame_data;
            self->bytes_left = frame_size;
            self->state = DECODING_BODY;
        }
    }

    if (self->state == DECODING_BODY) {
        const size_t n =
            iobuf_read (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;
        if (self->bytes_left == 0)
            self->state = FRAME_READY;
    }

    *info = (decoder_info_t) {
        .ready = self->state == FRAME_READY,
        .dba_size = self->state == DECODING_BODY ? self->bytes_left : 0
    };

    return 0;
}

static void *
s_buffer (void *self_)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    return self->state == DECODING_BODY? self->ptr: NULL;
}

static int
s_advance (void *self_, size_t n, decoder_info_t *info)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    if (self->state != DECODING_BODY)
        return -1;
    if (n > self->bytes_left)
        return -1;

    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left == 0)
        self->state = FRAME_READY;

    *info = (decoder_info_t) {
        .ready = self->state == FRAME_READY,
        .dba_size = self->bytes_left
    };

    return 0;
}

static frame_t *
s_decode (void *self_, decoder_info_t *info)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    frame_t *frame = self->frame;
    self->bytes_left = 1;
    self->ptr = self->buffer;
    self->frame = NULL;
    self->state = DECODING_FLAGS;

    *info = (decoder_info_t) { 0 };

    return frame;
}

static void
s_destroy (void **self_p)
{
    assert (self_p);
    if (*self_p) {
        zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) *self_p;
        frame_destroy (&self->frame);
        free (self);
        *self_p = NULL;
    }
}

static uint64_t
s_decode_length (const uint8_t *ptr)
{
    if ((ptr [0] & 0x02) == 0)
        return
            (uint64_t) ptr [1];
    else
        return
            (uint64_t) ptr [1] << 56 ||
            (uint64_t) ptr [2] << 48 ||
            (uint64_t) ptr [3] << 40 ||
            (uint64_t) ptr [4] << 32 ||
            (uint64_t) ptr [5] << 24 ||
            (uint64_t) ptr [6] << 16 ||
            (uint64_t) ptr [7] << 8  ||
            (uint64_t) ptr [8];
}

decoder_t *
zmtp_v3_decoder_create_decoder ()
{
    static struct decoder_ops ops = {
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
