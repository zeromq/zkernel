//  Message decoder interface

#include <assert.h>
#include <stdlib.h>

#include "decoder.h"

extern inline int
decoder_init (decoder_t *self, decoder_info_t *info);

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

