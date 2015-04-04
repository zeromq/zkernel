/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "pdu.h"
#include "zmtp_utils.h"
#include "zmtp_v2_frame_encoder.h"

#define WAITING_FOR_PDU     0
#define READING_HEADER      1
#define READING_BODY        2

struct zmtp_v2_frame_encoder {
    int state;
    uint8_t buffer [9];
    pdu_t *pdu;
    uint8_t *ptr;
    size_t bytes_left;
};

zmtp_v2_frame_encoder_t *
zmtp_v2_frame_encoder_new (zmtp_v2_frame_encoder_info_t *info)
{
    zmtp_v2_frame_encoder_t *self =
        (zmtp_v2_frame_encoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v2_frame_encoder_t) { .state = WAITING_FOR_PDU };
        *info = (zmtp_v2_frame_encoder_info_t) {
            .flags = ZMTP_V2_FRAME_ENCODER_READY
        };
    }

    return self;
}

int
zmtp_v2_frame_encoder_putmsg (zmtp_v2_frame_encoder_t *self,
    pdu_t *pdu, zmtp_v2_frame_encoder_info_t *info)
{
    assert (self);

    if (self->state != WAITING_FOR_PDU)
        return -1;

    assert (self->pdu == NULL);
    self->pdu = pdu;

    uint8_t *buffer = self->ptr = self->buffer;
    buffer [0] = 0;     // flags
    if (pdu->pdu_size > 255) {
        buffer [0] |= 0x02;
        put_uint64 (buffer + 1, pdu->pdu_size);
        self->bytes_left = 9;
    }
    else {
        buffer [1] = (uint8_t) pdu->pdu_size;
        self->bytes_left = 2;
    }

    *info = (zmtp_v2_frame_encoder_info_t) {
        .flags = ZMTP_V2_FRAME_ENCODER_READ_OK
    };

    return 0;
}

int
zmtp_v2_frame_encoder_read (zmtp_v2_frame_encoder_t *self,
    iobuf_t *iobuf, zmtp_v2_frame_encoder_info_t *info)
{
    assert (self);

    if (self->state == WAITING_FOR_PDU)
        return -1;

    assert (self->state == READING_HEADER
         || self->state == READING_BODY);

    if (self->state == READING_HEADER) {
        const size_t n =
            iobuf_write (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;

        if (self->bytes_left == 0) {
            assert (self->pdu);
            self->ptr = self->pdu->pdu_data;
            self->bytes_left = self->pdu->pdu_size;
            self->state = READING_BODY;
        }
    }

    if (self->state == READING_BODY) {
        const size_t n =
            iobuf_write (iobuf, self->ptr, self->bytes_left);
        self->ptr += n;
        self->bytes_left -= n;

        if (self->bytes_left == 0) {
            pdu_destroy (&self->pdu);
            self->state = WAITING_FOR_PDU;
        }
    }

    if (self->state == WAITING_FOR_PDU)
        *info = (zmtp_v2_frame_encoder_info_t) {
            .flags = ZMTP_V2_FRAME_ENCODER_READY
        };
    else
        *info = (zmtp_v2_frame_encoder_info_t) {
            .flags = ZMTP_V2_FRAME_ENCODER_READ_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

int
zmtp_v2_frame_encoder_advance (zmtp_v2_frame_encoder_t *self,
    size_t n, zmtp_v2_frame_encoder_info_t *info)
{
    assert (self);

    if (self->state == WAITING_FOR_PDU)
        return -1;

    assert (self->state == READING_HEADER
         || self->state == READING_BODY);

    if (n > self->bytes_left)
        return -1;

    self->ptr += n;
    self->bytes_left -= n;

    if (self->state == READING_HEADER) {
        if (self->bytes_left == 0) {
            pdu_t *pdu = self->pdu;
            assert (pdu);
            self->ptr = pdu->pdu_data;
            self->bytes_left = pdu->pdu_size;
            self->state = READING_BODY;
        }
    }

    if (self->bytes_left == 0) {
        pdu_destroy (&self->pdu);
        self->state = WAITING_FOR_PDU;
        *info = (zmtp_v2_frame_encoder_info_t) {
            .flags = ZMTP_V2_FRAME_ENCODER_READY
        };
    }
    else
        *info = (zmtp_v2_frame_encoder_info_t) {
            .flags = ZMTP_V2_FRAME_ENCODER_READ_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

void
zmtp_v2_frame_encoder_destroy (zmtp_v2_frame_encoder_t **self_p)
{
    if (*self_p) {
        zmtp_v2_frame_encoder_t *self = (zmtp_v2_frame_encoder_t *) *self_p;
        if (self->pdu)
            pdu_destroy (&self->pdu);
        free (self);
        *self_p = NULL;
    }
}
