//  Codec class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>

#include "protocol.h"

extern inline int
protocol_init (protocol_t *self, uint32_t *status);

extern inline int
protocol_encode (protocol_t *self, frame_t *frame, uint32_t *status);

extern inline int
protocol_read (protocol_t *self, iobuf_t *iobuf, uint32_t *status);

extern inline int
protocol_read_buffer (protocol_t *self, const void **buffer, size_t *buffer_size);

extern inline int
protocol_read_advance (protocol_t *self, size_t n, uint32_t *status);

extern inline frame_t *
protocol_decode (protocol_t *self, uint32_t *status);

extern inline int
protocol_write (protocol_t *self, iobuf_t *iobuf, uint32_t *status);

extern inline int
protocol_write_buffer (protocol_t *self, void **buffer, size_t *buffer_size);

extern inline int
protocol_write_advance (protocol_t *self, size_t n, uint32_t *status);

void
protocol_destroy (protocol_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        protocol_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
