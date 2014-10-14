//  Encoder class

#include <assert.h>
#include <stdlib.h>

#include "encoder.h"

extern inline int
encoder_frame (encoder_t *self, frame_t **frame_p);

extern inline int
encoder_encode (encoder_t *self, iobuf_t *iobuf);

extern inline int
encoder_error (encoder_t *self);

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
