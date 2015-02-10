//  Protocol engine class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>

#include "protocol_engine.h"

extern inline int
protocol_engine_init (protocol_engine_t *self, uint32_t *status);

extern inline int
protocol_engine_encode (protocol_engine_t *self, pdu_t *pdu, uint32_t *status);

extern inline int
protocol_engine_read (protocol_engine_t *self, iobuf_t *iobuf, uint32_t *status);

extern inline int
protocol_engine_read_buffer (protocol_engine_t *self, const void **buffer, size_t *buffer_size);

extern inline int
protocol_engine_read_advance (protocol_engine_t *self, size_t n, uint32_t *status);

extern inline pdu_t *
protocol_engine_decode (protocol_engine_t *self, uint32_t *status);

extern inline int
protocol_engine_write (protocol_engine_t *self, iobuf_t *iobuf, uint32_t *status);

extern inline int
protocol_engine_write_buffer (protocol_engine_t *self, void **buffer, size_t *buffer_size);

extern inline int
protocol_engine_write_advance (protocol_engine_t *self, size_t n, uint32_t *status);

void
protocol_engine_destroy (protocol_engine_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        protocol_engine_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
