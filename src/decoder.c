//  Message decoder interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "decoder.h"

extern inline void
decoder_info (decoder_t *self, decoder_info_t *info);

extern inline int
decoder_write (decoder_t *self, iobuf_t *iobuf, decoder_info_t *info);

extern inline void *
decoder_buffer (decoder_t *self);

extern inline int
decoder_advance (decoder_t *self, size_t n, decoder_info_t *info);

extern inline frame_t *
decoder_decode (decoder_t *self, decoder_info_t *info);

void
decoder_destroy (decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        decoder_t *self = *self_p;
        self->ops.destroy (&self->object);
        free (self);
        *self_p = NULL;
    }
}

