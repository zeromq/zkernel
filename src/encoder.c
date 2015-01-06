//  Encoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "encoder.h"

extern inline void
encoder_info (encoder_t *self, encoder_info_t *info);

extern inline int
encoder_encode (encoder_t *self, frame_t *frame, encoder_info_t *info);

extern inline int
encoder_read (encoder_t *self, iobuf_t *iobuf, encoder_info_t *info);

extern inline const void *
encoder_buffer (encoder_t *self);

extern inline int
encoder_advance (encoder_t *self, size_t n, encoder_info_t *info);

void
encoder_destroy (encoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        encoder_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
