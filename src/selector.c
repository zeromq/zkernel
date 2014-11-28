//  Selector class

#include <assert.h>
#include <stdlib.h>

#include "selector.h"

extern inline bool
selector_is_handshake_complete (selector_t *self);

extern inline encoder_t *
selector_encoder (selector_t *self, encoder_info_t *encoder_info);

extern inline decoder_t *
selector_decoder (selector_t *self, decoder_info_t *decoder_info);

extern inline int
selector_handshake (selector_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf);

void
selector_destroy (selector_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        selector_t *self = *self_p;
        self->destroy (self_p);
    }
}
