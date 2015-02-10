//  Message decoder interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __DECODER_H_INCLUDED__
#define __DECODER_H_INCLUDED__

#include <stdint.h>

#include "iobuf.h"
#include "pdu.h"

#define DECODER_BUFFER_MASK 0xfffff000
#define DECODER_READY       0x01
#define DECODER_WRITE_OK    0x02
#define DECODER_ERROR       0x04

typedef uint32_t decoder_status_t;

struct decoder;

struct decoder_ops {
    int (*write) (struct decoder *self, iobuf_t *iobuf, decoder_status_t *status);
    int (*buffer) (struct decoder *self, void **buffer, size_t *buffer_size);
    int (*advance) (struct decoder *self, size_t n, decoder_status_t *status);
    pdu_t *(*decode) (struct decoder *self, decoder_status_t *status);
    decoder_status_t (*status) (struct decoder *self);
    void (*destroy) (struct decoder **self_p);
};

struct decoder {
    struct decoder_ops ops;
};

typedef struct decoder decoder_t;

typedef decoder_t *decoder_constructor_t ();

inline pdu_t *
decoder_decode (decoder_t *self, decoder_status_t *status)
{
    return self->ops.decode (self, status);
}

inline int
decoder_write (decoder_t *self, iobuf_t *iobuf, decoder_status_t *status)
{
    return self->ops.write (self, iobuf, status);
}

inline int
decoder_buffer (decoder_t *self, void **buffer, size_t *buffer_size)
{
    return self->ops.buffer (self, buffer, buffer_size);
}

inline int
decoder_advance (decoder_t *self, size_t n, decoder_status_t *status)
{
    return self->ops.advance (self, n, status);
}

inline decoder_status_t
decoder_status (decoder_t *self)
{
    return self->ops.status (self);
}

void
    decoder_destroy (decoder_t **self_p);

#endif
