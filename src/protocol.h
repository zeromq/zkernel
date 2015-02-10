//  Protocol engine class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __PROTOCOL_H_INCLUDED__
#define __PROTOCOL_H_INCLUDED__

#include <stdint.h>

#include "iobuf.h"
#include "pdu.h"

#define ZKERNEL_PROTOCOL_ENCODER_READY     0x01
#define ZKERNEL_PROTOCOL_READ_OK           0x02
#define ZKERNEL_PROTOCOL_DECODER_READY     0x04
#define ZKERNEL_PROTOCOL_WRITE_OK          0x08
#define ZKERNEL_PROTOCOL_ERROR             0x10

struct protocol;

struct protocol_ops {
    int (*init) (struct protocol *self, uint32_t *status);
    int (*encode) (struct protocol *self, pdu_t *pdu, uint32_t *status);
    int (*read) (struct protocol *self, iobuf_t *iobuf, uint32_t *status);
    int (*read_buffer) (struct protocol *self, const void **buffer, size_t *buffer_size);
    int (*read_advance) (struct protocol *self, size_t n, uint32_t *status);
    pdu_t *(*decode) (struct protocol *self, uint32_t *status);
    int (*write) (struct protocol *self, iobuf_t *iobuf, uint32_t *status);
    int (*write_buffer) (struct protocol *self, void **buffer, size_t *buffer_size);
    int (*write_advance) (struct protocol *self, size_t n, uint32_t *status);
    void (*destroy) (struct protocol **self_p);
};

struct protocol {
    struct protocol_ops ops;
};

typedef struct protocol protocol_t;

typedef protocol_t *(protocol_constructor_t) ();

inline int
protocol_init (protocol_t *self, uint32_t *status)
{
    return self->ops.init (self, status);
}

inline int
protocol_encode (protocol_t *self, pdu_t *pdu, uint32_t *status)
{
    return self->ops.encode (self, pdu, status);
}

inline int
protocol_read (protocol_t *self, iobuf_t *iobuf, uint32_t *status)
{
    return self->ops.read (self, iobuf, status);
}

inline int
protocol_read_buffer (protocol_t *self, const void **buffer, size_t *buffer_size)
{
    return self->ops.read_buffer (self, buffer, buffer_size);
}

inline int
protocol_read_advance (protocol_t *self, size_t n, uint32_t *status)
{
    return self->ops.read_advance (self, n, status);
}

inline pdu_t *
protocol_decode (protocol_t *self, uint32_t *status)
{
    return self->ops.decode (self, status);
}

inline int
protocol_write (protocol_t *self, iobuf_t *iobuf, uint32_t *status)
{
    return self->ops.write (self, iobuf, status);
}

inline int
protocol_write_buffer (protocol_t *self, void **buffer, size_t *buffer_size)
{
    return self->ops.write_buffer (self, buffer, buffer_size);
}

inline int
protocol_write_advance (protocol_t *self, size_t n, uint32_t *status)
{
    return self->ops.write_advance (self, n, status);
}

void
    protocol_destroy (protocol_t **self_p);

#endif
