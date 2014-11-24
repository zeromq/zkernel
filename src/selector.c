//  Selector class

#include <assert.h>
#include <stdlib.h>

#include "selector.h"

extern inline bool
selector_in_handshake (selector_t *self);

extern inline int
selector_select (selector_t *self, encoder_t **encoder, decoder_t **decoder);

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
