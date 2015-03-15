//  ZMTP v3 decoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZMTP_V3_DECODER_H_INCLUDED__
#define __ZMTP_V3_DECODER_H_INCLUDED__

#define ZMTP_V3_DECODER_READY           0x01
#define ZMTP_V3_DECODER_WRITE_OK        0x02
#define ZMTP_V3_DECODER_BUFFER_FLAG     0x04

typedef struct zmtp_v3_decoder zmtp_v3_decoder_t;

typedef unsigned int zmtp_v3_decoder_status_t;

zmtp_v3_decoder_t *
    zmtp_v3_decoder_new (zmtp_v3_decoder_status_t *status);

int
    zmtp_v3_decoder_write (
        zmtp_v3_decoder_t *self, iobuf_t *iobuf, zmtp_v3_decoder_status_t *status);

int
    zmtp_v3_decoder_buffer (
        zmtp_v3_decoder_t *self, void **buffer, size_t *buffer_size);

int
    zmtp_v3_decoder_advance (
        zmtp_v3_decoder_t *self, size_t n, zmtp_v3_decoder_status_t *status);

pdu_t *
    zmtp_v3_decoder_getmsg (
        zmtp_v3_decoder_t *self, zmtp_v3_decoder_status_t *status);

void
    zmtp_v3_decoder_set_buffer_threshold (
        zmtp_v3_decoder_t *self, size_t buffer_threshold, zmtp_v3_decoder_status_t *status);

void
    zmtp_v3_decoder_destroy (zmtp_v3_decoder_t **self_p);

#endif
