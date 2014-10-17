//  Message decoder interface

#include <assert.h>
#include <stdlib.h>

#include "msg_decoder.h"

extern inline int
msg_decoder_write (
    msg_decoder_t *self, iobuf_t *iobuf, msg_decoder_info_t *info);

extern inline uint8_t *
msg_decoder_buffer (msg_decoder_t *self);

extern inline int
msg_decoder_advance (msg_decoder_t *self, size_t n, msg_decoder_info_t *info);

extern inline frame_t *
msg_decoder_decode (msg_decoder_t *self, msg_decoder_info_t *info);

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

