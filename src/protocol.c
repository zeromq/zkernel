//  Protocol class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "protocol.h"

extern inline bool
protocol_is_handshake_complete (protocol_t *self);

extern inline encoder_t *
protocol_encoder (protocol_t *self, encoder_info_t *encoder_info);

extern inline decoder_t *
protocol_decoder (protocol_t *self, decoder_info_t *decoder_info);

extern inline int
protocol_handshake (protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf);

extern inline size_t
protocol_min_buffer_size (protocol_t *self);

void
protocol_destroy (protocol_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        protocol_t *self = *self_p;
        self->destroy (self_p);
    }
}
