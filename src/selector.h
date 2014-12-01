//  Selector class

#ifndef __SELECTOR_H_INCLUDED__
#define __SELECTOR_H_INCLUDED__

#include <stdbool.h>

#include "iobuf.h"
#include "encoder.h"
#include "decoder.h"

struct selector {
    bool (*is_handshake_complete) (struct selector *self);
    encoder_t * (*encoder) (struct selector *self, encoder_info_t *encoder_info);
    decoder_t * (*decoder) (struct selector *self, decoder_info_t *decoder_info);
    int (*handshake) (struct selector *self, iobuf_t *recvbuf, iobuf_t *sendbuf);
    size_t (*min_buffer_size) (struct selector *self);
    void (*destroy) (struct selector **self_p);
};

typedef struct selector selector_t;

inline bool
selector_is_handshake_complete (selector_t *self)
{
    return self->is_handshake_complete (self);
}

inline encoder_t *
selector_encoder (selector_t *self, encoder_info_t *encoder_info)
{
    return self->encoder (self, encoder_info);
}

inline decoder_t *
selector_decoder (selector_t *self, decoder_info_t *decoder_info)
{
    return self->decoder (self, decoder_info);
}

inline int
selector_handshake (selector_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    return self->handshake (self, recvbuf, sendbuf);
}

inline size_t
selector_min_buffer_size (selector_t *self)
{
    return self->min_buffer_size (self);
}

void
    selector_destroy (selector_t **self_p);

#endif
