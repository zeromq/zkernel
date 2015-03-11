//  Protocol engine class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __PROTOCOL_ENGINE_H_INCLUDED__
#define __PROTOCOL_ENGINE_H_INCLUDED__

#include <stdint.h>

#include "iobuf.h"
#include "pdu.h"

#define ZKERNEL_PROTOCOL_ENGINE_ENCODER_READY     0x01
#define ZKERNEL_PROTOCOL_ENGINE_READ_OK           0x02
#define ZKERNEL_PROTOCOL_ENGINE_DECODER_READY     0x04
#define ZKERNEL_PROTOCOL_ENGINE_WRITE_OK          0x08
#define ZKERNEL_PROTOCOL_ENGINE_ERROR             0x10
#define ZKERNEL_PROTOCOL_ENGINE_STOPPED         0x20

struct protocol_engine;

struct protocol_engine_ops {
    int (*init) (struct protocol_engine *self, uint32_t *status);
    int (*encode) (struct protocol_engine *self, pdu_t *pdu, uint32_t *status);
    int (*read) (struct protocol_engine *self, iobuf_t *iobuf, uint32_t *status);
    int (*read_buffer) (struct protocol_engine *self, const void **buffer, size_t *buffer_size);
    int (*read_advance) (struct protocol_engine *self, size_t n, uint32_t *status);
    pdu_t *(*decode) (struct protocol_engine *self, uint32_t *status);
    int (*write) (struct protocol_engine *self, iobuf_t *iobuf, uint32_t *status);
    int (*write_buffer) (struct protocol_engine *self, void **buffer, size_t *buffer_size);
    int (*write_advance) (struct protocol_engine *self, size_t n, uint32_t *status);
    int (*next) (struct protocol_engine **self_p, uint32_t *status);
    void (*destroy) (struct protocol_engine **self_p);
};

struct protocol_engine {
    struct protocol_engine_ops ops;
};

typedef struct protocol_engine protocol_engine_t;

typedef protocol_engine_t *(protocol_engine_constructor_t) ();

inline int
protocol_engine_init (protocol_engine_t *self, uint32_t *status)
{
    return self->ops.init (self, status);
}

inline int
protocol_engine_encode (protocol_engine_t *self, pdu_t *pdu, uint32_t *status)
{
    return self->ops.encode (self, pdu, status);
}

inline int
protocol_engine_read (protocol_engine_t *self, iobuf_t *iobuf, uint32_t *status)
{
    return self->ops.read (self, iobuf, status);
}

inline int
protocol_engine_read_buffer (protocol_engine_t *self, const void **buffer, size_t *buffer_size)
{
    return self->ops.read_buffer (self, buffer, buffer_size);
}

inline int
protocol_engine_read_advance (protocol_engine_t *self, size_t n, uint32_t *status)
{
    return self->ops.read_advance (self, n, status);
}

inline pdu_t *
protocol_engine_decode (protocol_engine_t *self, uint32_t *status)
{
    return self->ops.decode (self, status);
}

inline int
protocol_engine_write (protocol_engine_t *self, iobuf_t *iobuf, uint32_t *status)
{
    return self->ops.write (self, iobuf, status);
}

inline int
protocol_engine_write_buffer (protocol_engine_t *self, void **buffer, size_t *buffer_size)
{
    return self->ops.write_buffer (self, buffer, buffer_size);
}

inline int
protocol_engine_write_advance (protocol_engine_t *self, size_t n, uint32_t *status)
{
    return self->ops.write_advance (self, n, status);
}

int
    protocol_engine_next (protocol_engine_t **self_p, uint32_t *status);

void
    protocol_engine_destroy (protocol_engine_t **self_p);

#endif
