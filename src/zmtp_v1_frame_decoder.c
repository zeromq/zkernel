//  ZMTP v3 frame decoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "pdu.h"
#include "zmtp_utils.h"
#include "zmtp_v1_frame_decoder.h"

struct zmtp_v1_frame_decoder {
    int state;
    uint8_t buffer [8];
    uint64_t frame_size;
    pdu_t *pdu;
    uint8_t *ptr;
    size_t bytes_left;
};

#define DECODING_LENGTH     0
#define DECODING_LENGTH2    1
#define DECODING_FLAGS      2
#define DECODING_BODY       3
#define FRAME_READY         4

zmtp_v1_frame_decoder_t *
zmtp_v1_frame_decoder_new (zmtp_v1_frame_decoder_info_t *info)
{
    zmtp_v1_frame_decoder_t *self =
        (zmtp_v1_frame_decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v1_frame_decoder_t) {
            .state = DECODING_LENGTH,
        };
        *info = (zmtp_v1_frame_decoder_info_t) {
            .flags = ZMTP_V1_FRAME_DECODER_WRITE_OK,
        };
    }

    return self;
}

int
zmtp_v1_frame_decoder_write (zmtp_v1_frame_decoder_t *self,
    iobuf_t *iobuf, zmtp_v1_frame_decoder_info_t *info)
{
    assert (self);

    if (self->state == FRAME_READY)
        return -1;

    if (self->state == DECODING_LENGTH) {
        const size_t n =
            iobuf_read (iobuf, self->buffer, 1);
        if (n == 1) {
            if (self->buffer [0] == 0xff) {
                self->ptr = self->buffer;
                self->bytes_left = 8;
                self->state = DECODING_LENGTH2;
            }
            else {
                if (self->buffer [0] == 0)
                    return -1;
                self->frame_size = self->buffer [0] - 1;
                self->state = DECODING_FLAGS;
            }
        }
    }

    if (self->state == DECODING_LENGTH2) {
        const size_t n =
            iobuf_read (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;
        if (self->bytes_left == 0) {
            uint64_t length = get_uint64 (self->buffer);
            if (length == 0)
                return -1;
            self->frame_size = length - 1;
            self->state = DECODING_FLAGS;
        }
    }

    if (self->state == DECODING_FLAGS) {
        const size_t n =
            iobuf_read (iobuf, self->buffer, 1);
        if (n == 1) {
            // process flags
            self->pdu = pdu_new_with_size (self->frame_size);
            if (self->pdu == NULL)
                return -1;
            self->ptr = self->pdu->pdu_data;
            self->bytes_left = self->frame_size;
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

    if (self->state == FRAME_READY)
        *info = (zmtp_v1_frame_decoder_info_t) {
            .flags = ZMTP_V1_FRAME_DECODER_READY,
        };
    if (self->state == DECODING_BODY)
        *info = (zmtp_v1_frame_decoder_info_t) {
            .flags = ZMTP_V1_FRAME_DECODER_WRITE_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };
    else
        *info = (zmtp_v1_frame_decoder_info_t) {
            .flags = ZMTP_V1_FRAME_DECODER_WRITE_OK,
        };

    return 0;
}

int
zmtp_v1_frame_decoder_advance (zmtp_v1_frame_decoder_t *self,
    size_t n, zmtp_v1_frame_decoder_info_t *info)
{
    assert (self);

    if (self->state != DECODING_BODY)
        return -1;
    if (n > self->bytes_left)
        return -1;

    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left == 0)
        self->state = FRAME_READY;

    if (self->state == FRAME_READY)
        *info = (zmtp_v1_frame_decoder_info_t) {
            .flags = ZMTP_V1_FRAME_DECODER_READY,
        };
    if (self->state == DECODING_BODY)
        *info = (zmtp_v1_frame_decoder_info_t) {
            .flags = ZMTP_V1_FRAME_DECODER_WRITE_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

pdu_t *
zmtp_v1_frame_decoder_getmsg (zmtp_v1_frame_decoder_t *self,
    zmtp_v1_frame_decoder_info_t *info)
{
    assert (self);

    if (self->state != FRAME_READY)
        return NULL;

    assert (self->pdu);
    pdu_t *pdu = self->pdu;
    self->pdu = NULL;
    self->state = DECODING_LENGTH;

    *info = (zmtp_v1_frame_decoder_info_t) {
        .flags = ZMTP_V1_FRAME_DECODER_WRITE_OK,
    };

    return pdu;
}

void
zmtp_v1_frame_decoder_destroy (zmtp_v1_frame_decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zmtp_v1_frame_decoder_t *self = *self_p;
        if (self->pdu)
            pdu_destroy (&self->pdu);
        free (self);
        *self_p = NULL;
    }
}
