//  ZMTP v3 decoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "pdu.h"
#include "zmtp_utils.h"
#include "zmtp_v2_frame_decoder.h"

struct zmtp_v2_frame_decoder {
    int state;
    uint8_t buffer [9];
    pdu_t *pdu;
    uint8_t *ptr;
    size_t bytes_left;
};

typedef struct zmtp_v2_frame_decoder zmtp_v2_frame_decoder_t;

#define DECODING_FLAGS      0
#define DECODING_LENGTH     1
#define DECODING_BODY       2
#define PDU_READY           3

static size_t
s_decode_length (const uint8_t *ptr);

zmtp_v2_frame_decoder_t *
zmtp_v2_frame_decoder_new (zmtp_v2_frame_decoder_info_t *info)
{
    zmtp_v2_frame_decoder_t *self =
        (zmtp_v2_frame_decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v2_frame_decoder_t) {
            .state = DECODING_FLAGS,
            .ptr = self->buffer,
            .bytes_left = 1
        };
        *info = (zmtp_v2_frame_decoder_info_t) {
            .flags = ZMTP_V2_FRAME_DECODER_WRITE_OK
        };
    }
    return self;
}

int
zmtp_v2_frame_decoder_write (zmtp_v2_frame_decoder_t *self,
    iobuf_t *iobuf, zmtp_v2_frame_decoder_info_t *info)
{
    assert (self);

    if (self->state == PDU_READY)
        return -1;

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

    if (self->state == PDU_READY)
        *info = (zmtp_v2_frame_decoder_info_t) {
            .flags = ZMTP_V2_FRAME_DECODER_READY
        };
    else
        *info = (zmtp_v2_frame_decoder_info_t) {
            .flags = ZMTP_V2_FRAME_DECODER_WRITE_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

int
zmtp_v2_frame_decoder_advance (
    zmtp_v2_frame_decoder_t *self, size_t n, zmtp_v2_frame_decoder_info_t *info)
{
    assert (self);

    if (self->state != DECODING_BODY)
        return -1;
    if (n > self->bytes_left)
        return -1;

    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left == 0)
        self->state = PDU_READY;

    if (self->state == PDU_READY)
        *info = (zmtp_v2_frame_decoder_info_t) {
            .flags = ZMTP_V2_FRAME_DECODER_READY
        };
    else
        *info = (zmtp_v2_frame_decoder_info_t) {
            .flags = ZMTP_V2_FRAME_DECODER_WRITE_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

pdu_t *
zmtp_v2_frame_decoder_getmsg (
    zmtp_v2_frame_decoder_t *self, zmtp_v2_frame_decoder_info_t *info)
{
    assert (self);

    if (self->state != PDU_READY)
        return NULL;

    pdu_t *pdu = self->pdu;
    self->pdu = NULL;
    self->state = DECODING_FLAGS;
    self->ptr = self->buffer;
    self->bytes_left = 1;

    *info = (zmtp_v2_frame_decoder_info_t) {
        .flags = ZMTP_V2_FRAME_DECODER_WRITE_OK
    };

    return pdu;
}

void
zmtp_v2_frame_decoder_destroy (zmtp_v2_frame_decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zmtp_v2_frame_decoder_t *self = (zmtp_v2_frame_decoder_t *) *self_p;
        pdu_destroy (&self->pdu);
        free (self);
        *self_p = NULL;
    }
}

static uint64_t
s_decode_length (const uint8_t *ptr)
{
    if ((ptr [0] & 0x02) == 0)
        return (uint64_t) ptr [1];
    else
        return get_uint64 (ptr + 1);
}
