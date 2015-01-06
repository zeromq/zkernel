//  Protocol class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __PROTOCOL_H_INCLUDED__
#define __PROTOCOL_H_INCLUDED__

#include <stdbool.h>

#include "iobuf.h"
#include "encoder.h"
#include "decoder.h"

struct protocol {
    bool (*is_handshake_complete) (struct protocol *self);
    encoder_t * (*encoder) (struct protocol *self, encoder_info_t *encoder_info);
    decoder_t * (*decoder) (struct protocol *self, decoder_info_t *decoder_info);
    int (*handshake) (struct protocol *self, iobuf_t *recvbuf, iobuf_t *sendbuf);
    size_t (*min_buffer_size) (struct protocol *self);
    void (*destroy) (struct protocol **self_p);
};

typedef struct protocol protocol_t;

inline bool
protocol_is_handshake_complete (protocol_t *self)
{
    return self->is_handshake_complete (self);
}

inline encoder_t *
protocol_encoder (protocol_t *self, encoder_info_t *encoder_info)
{
    return self->encoder (self, encoder_info);
}

inline decoder_t *
protocol_decoder (protocol_t *self, decoder_info_t *decoder_info)
{
    return self->decoder (self, decoder_info);
}

inline int
protocol_handshake (protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    return self->handshake (self, recvbuf, sendbuf);
}

inline size_t
protocol_min_buffer_size (protocol_t *self)
{
    return self->min_buffer_size (self);
}

void
    protocol_destroy (protocol_t **self_p);

#endif
