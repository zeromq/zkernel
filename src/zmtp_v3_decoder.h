//  ZMTP v3 decoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZMTP_V3_DECODER_H_INCLUDED__
#define __ZMTP_V3_DECODER_H_INCLUDED__

#define ZMTP_V3_DECODER_READY           0x01
#define ZMTP_V3_DECODER_WRITE_OK        0x02

typedef struct zmtp_v3_decoder zmtp_v3_decoder_t;

struct zmtp_v3_decoder_info {
    unsigned int flags;
    size_t dba_size;
    uint8_t *dba_ptr;
};

typedef struct zmtp_v3_decoder_info zmtp_v3_decoder_info_t;

zmtp_v3_decoder_t *
    zmtp_v3_decoder_new (zmtp_v3_decoder_info_t *info);

int
    zmtp_v3_decoder_write (
        zmtp_v3_decoder_t *self, iobuf_t *iobuf, zmtp_v3_decoder_info_t *info);

int
    zmtp_v3_decoder_advance (
        zmtp_v3_decoder_t *self, size_t n, zmtp_v3_decoder_info_t *info);

pdu_t *
    zmtp_v3_decoder_getmsg (
        zmtp_v3_decoder_t *self, zmtp_v3_decoder_info_t *info);

void
    zmtp_v3_decoder_destroy (zmtp_v3_decoder_t **self_p);

#endif
