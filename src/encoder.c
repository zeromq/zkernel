//  Encoder class

#include <assert.h>
#include <stdlib.h>

#include "encoder.h"

extern inline int
encoder_encode (encoder_t *self, frame_t *frame, encoder_info_t *info);

extern inline int
encoder_read (encoder_t *self, iobuf_t *iobuf, encoder_info_t *info);

extern inline int
encoder_buffer (encoder_t *self, iobuf_t *iobuf, encoder_info_t *info);

void
encoder_destroy (encoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        encoder_t *self = *self_p;
        self->ops.destroy (&self->object);
        free (self);
        *self_p = NULL;
    }
}
