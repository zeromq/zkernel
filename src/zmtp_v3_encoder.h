/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "pdu.h"
#include "iobuf.h"

#ifndef __ZMTP_V3_ENCODER_H_INCLUDED__
#define __ZMTP_V3_ENCODER_H_INCLUDED__

#define ZMTP_V3_ENCODER_READY           0x01
#define ZMTP_V3_ENCODER_READ_OK         0x02

typedef struct zmtp_v3_encoder zmtp_v3_encoder_t;

struct zmtp_v3_encoder_info {
    unsigned int flags;
    size_t dba_size;
};

typedef struct zmtp_v3_encoder_info zmtp_v3_encoder_info_t;

zmtp_v3_encoder_t *
    zmtp_v3_encoder_new (zmtp_v3_encoder_info_t *info);

int
    zmtp_v3_encoder_putmsg (
        zmtp_v3_encoder_t *self, pdu_t *pdu, zmtp_v3_encoder_info_t *info);

int
    zmtp_v3_encoder_read (
        zmtp_v3_encoder_t *self, iobuf_t *iobuf, zmtp_v3_encoder_info_t *info);

int
    zmtp_v3_encoder_buffer (
        zmtp_v3_encoder_t *self, const void **buffer, size_t *buffer_size);

int
    zmtp_v3_encoder_advance (
        zmtp_v3_encoder_t *self, size_t n, zmtp_v3_encoder_info_t *info);

void
    zmtp_v3_encoder_destroy (zmtp_v3_encoder_t **base_p);

#endif
