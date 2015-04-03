/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "pdu.h"
#include "iobuf.h"
#include "zmtp_utils.h"
#include "zmtp_v1_frame_encoder.h"

#define WAITING_FOR_PDU     0
#define READING_HEADER      1
#define READING_BODY        2

struct zmtp_v1_frame_encoder {
    int state;
    uint8_t buffer [10];
    pdu_t *pdu;
    uint8_t *ptr;
    size_t bytes_left;
};

zmtp_v1_frame_encoder_t *
zmtp_v1_frame_encoder_new (zmtp_v1_frame_encoder_info_t *info)
{
    zmtp_v1_frame_encoder_t *self =
        (zmtp_v1_frame_encoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v1_frame_encoder_t) {
            .state = WAITING_FOR_PDU,
        };
        *info = (zmtp_v1_frame_encoder_info_t) {
            .flags = ZMTP_V1_FRAME_ENCODER_READY,
        };
    }

    return self;
}

int
zmtp_v1_frame_encoder_putmsg (zmtp_v1_frame_encoder_t *self,
    pdu_t *pdu, zmtp_v1_frame_encoder_info_t *info)
{
    assert (self);
    assert (self->state == WAITING_FOR_PDU);
    assert (self->pdu == NULL);

    self->pdu = pdu;
    uint8_t *buffer = self->ptr = self->buffer;

    if (pdu->pdu_size < 255) {
        buffer [0] = pdu->pdu_size + 1;
        buffer [1] = 0; // flags
        self->bytes_left = 2;
    }
    else {
        buffer [0] = 0xff;
        put_uint64 (buffer + 1, pdu->pdu_size + 1);
        buffer [9] = 0; // flags
        self->bytes_left = 10;
    }

    *info = (zmtp_v1_frame_encoder_info_t) {
        .flags = ZMTP_V1_FRAME_ENCODER_READ_OK,
    };

    return 0;
}

int
zmtp_v1_frame_encoder_read (zmtp_v1_frame_encoder_t *self,
    iobuf_t *iobuf, zmtp_v1_frame_encoder_info_t *info)
{
    assert (self);
    assert (self->state == READING_HEADER || self->state == READING_BODY);

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

    if (self->state == WAITING_FOR_PDU || self->state == READING_HEADER)
        *info = (zmtp_v1_frame_encoder_info_t) {
            .flags = ZMTP_V1_FRAME_ENCODER_READY,
        };
    else
        *info = (zmtp_v1_frame_encoder_info_t) {
            .flags = ZMTP_V1_FRAME_ENCODER_READ_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

int
zmtp_v1_frame_encoder_advance (zmtp_v1_frame_encoder_t *self,
    size_t n, zmtp_v1_frame_encoder_info_t *info)
{
    if (self == NULL)
        return -1;
    if (self->state != READING_BODY)
        return -1;
    if (n > self->bytes_left)
        return -1;

    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left == 0) {
        pdu_destroy (&self->pdu);
        self->state = WAITING_FOR_PDU;
        *info = (zmtp_v1_frame_encoder_info_t) {
            .flags = ZMTP_V1_FRAME_ENCODER_READY,
        };
    }
    else
        *info = (zmtp_v1_frame_encoder_info_t) {
            .flags = ZMTP_V1_FRAME_ENCODER_READ_OK,
            .buffer = self->ptr,
            .buffer_size = self->bytes_left,
        };

    return 0;
}

void
zmtp_v1_frame_encoder_destroy (zmtp_v1_frame_encoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zmtp_v1_frame_encoder_t *self = *self_p;
        if (self->pdu)
            pdu_destroy (&self->pdu);
        free (self);
        *self_p = NULL;
    }
}
