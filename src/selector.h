//  Selector class

#ifndef __SELECTOR_H_INCLUDED__
#define __SELECTOR_H_INCLUDED__

#include <stdbool.h>

#include "iobuf.h"
#include "encoder.h"
#include "decoder.h"

struct selector {
    bool (*ready) (struct selector *self);
    int (*select) (struct selector *self, encoder_t **encoder, decoder_t **decoder);
    int (*handshake) (struct selector *self, iobuf_t *recvbuf, iobuf_t *sendbuf);
    void (*destroy) (struct selector **self_p);
};

typedef struct selector selector_t;

extern bool
    selector_ready (selector_t *self);

extern int
    selector_select (selector_t *self, encoder_t **encoder, decoder_t **decoder);

extern int
    selector_handshake (selector_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf);

extern void
    selector_destroy (selector_t **self_p);

#endif
