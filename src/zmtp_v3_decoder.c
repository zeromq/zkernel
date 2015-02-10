//  ZMTP v3 decoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "pdu.h"
#include "decoder.h"

struct zmtp_v3_decoder {
    decoder_t base;
    int state;
    uint8_t buffer [9];
    pdu_t *pdu;
    uint8_t *ptr;
    size_t bytes_left;
};

typedef struct zmtp_v3_decoder zmtp_v3_decoder_t;

#define DECODING_FLAGS      0
#define DECODING_LENGTH     1
#define DECODING_BODY       2
#define PDU_READY           3

static size_t
s_decode_length (const uint8_t *ptr);

static struct decoder_ops decoder_ops;

static zmtp_v3_decoder_t *
s_new ()
{
    zmtp_v3_decoder_t *self =
        (zmtp_v3_decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v3_decoder_t) {
            .base = (decoder_t) { .ops = decoder_ops },
            .state = DECODING_FLAGS,
            .ptr = self->buffer,
            .bytes_left = 1
        };
    }
    return self;
}

static int
s_write (decoder_t *self_, iobuf_t *iobuf, decoder_status_t *status)
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
            const size_t pdu_size = (size_t) s_decode_length (self->buffer);
            self->pdu = pdu_new_with_size (pdu_size);
            if (self->pdu == NULL)
                return -1;
            self->ptr = self->pdu->pdu_data;
            self->bytes_left = pdu_size;
            self->state = DECODING_BODY;
        }
    }

    if (self->state == DECODING_BODY) {
        const size_t n =
            iobuf_read (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;
        if (self->bytes_left == 0)
            self->state = PDU_READY;
    }

    *status = 0;
    if (self->state == DECODING_BODY)
        *status |= self->bytes_left & DECODER_BUFFER_MASK;
    if (self->state == PDU_READY)
        *status |= DECODER_READY;
    else
        *status |= DECODER_WRITE_OK;

    return 0;
}

static int
s_buffer (decoder_t *self_, void **buffer, size_t *buffer_size)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    if (self->state != DECODING_BODY)
        return -1;
    else {
        *buffer = self->ptr;
        *buffer_size = self->bytes_left;
        return 0;
    }
}

static int
s_advance (decoder_t *self_, size_t n, decoder_status_t *status)
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
        self->state = PDU_READY;

    *status = 0;
    if (self->state == DECODING_BODY)
        *status |= self->bytes_left & DECODER_BUFFER_MASK;
    if (self->state == PDU_READY)
        *status |= DECODER_READY;
    else
        *status |= DECODER_WRITE_OK;

    return 0;
}

static pdu_t *
s_decode (decoder_t *self_, decoder_status_t *status)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    if (self->state != PDU_READY)
        return NULL;

    pdu_t *pdu = self->pdu;
    self->pdu = NULL;
    self->state = DECODING_FLAGS;
    self->ptr = self->buffer;
    self->bytes_left = 1;

    *status = DECODER_WRITE_OK;

    return pdu;
}

static decoder_status_t
s_status (decoder_t *self_)
{
    zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) self_;
    assert (self);

    if (self->state == DECODING_LENGTH && self->bytes_left == 0)
        return DECODER_ERROR;
    else {
        decoder_status_t status = 0;
        if (self->state == DECODING_BODY)
            status |= self->bytes_left & DECODER_BUFFER_MASK;
        if (self->state == PDU_READY)
            status |= DECODER_READY;
        else
            status |= DECODER_WRITE_OK;
        return status;
    }
}

static void
s_destroy (decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zmtp_v3_decoder_t *self = (zmtp_v3_decoder_t *) *self_p;
        pdu_destroy (&self->pdu);
        free (self);
        *self_p = NULL;
    }
}

static struct decoder_ops ops = {
    .write = s_write,
    .buffer = s_buffer,
    .advance = s_advance,
    .decode = s_decode,
    .status = s_status,
    .destroy = s_destroy
};

decoder_t *
zmtp_v3_decoder_create_decoder ()
{
    return (decoder_t *) s_new ();
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

