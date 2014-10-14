#include <assert.h>
#include <stdlib.h>

#include "msg_decoder.h"

extern inline int
msg_decoder_buffer (msg_decoder_t *self, iobuf_t *iobuf);

extern inline int
msg_decoder_decode (
    msg_decoder_t *self, iobuf_t *iobuf, msg_decoder_result_t *res);

int
msg_decoder_error (msg_decoder_t *self)
{
    return self->ops.error (self->object);
}

void
msg_decoder_destroy (msg_decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        msg_decoder_t *self = *self_p;
        self->ops.destroy (&self->object);
        free (self);
        *self_p = NULL;
    }
}

