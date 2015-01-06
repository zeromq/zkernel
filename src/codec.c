//  Codec class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>

#include "codec.h"

extern inline int
codec_init (codec_t *self, uint32_t *status);

extern inline int
codec_encode (codec_t *self, frame_t *frame, uint32_t *status);

extern inline int
codec_read (codec_t *self, iobuf_t *iobuf, uint32_t *status);

extern inline int
codec_read_buffer (codec_t *self, const void **buffer, size_t *buffer_size);

extern inline int
codec_read_advance (codec_t *self, size_t n, uint32_t *status);

extern inline frame_t *
codec_decode (codec_t *self, uint32_t *status);

extern inline int
codec_write (codec_t *self, iobuf_t *iobuf, uint32_t *status);

extern inline int
codec_write_buffer (codec_t *self, void **buffer, size_t *buffer_size);

extern inline int
codec_write_advance (codec_t *self, size_t n, uint32_t *status);

void
codec_destroy (codec_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        codec_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
