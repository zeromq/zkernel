// Encoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ENCODER_H_INCLUDED__
#define __ENCODER_H_INCLUDED__

#include <stdint.h>

#include "pdu.h"
#include "iobuf.h"

#define ZKERNEL_ENCODER_BUFFER_MASK 0xfffff000
#define ZKERNEL_ENCODER_READY       0x01
#define ZKERNEL_ENCODER_READ_OK     0x02
#define ZKERNEL_ENCODER_ERROR       0x04

typedef uint32_t encoder_status_t;

struct encoder;

struct encoder_ops {
    int (*encode) (struct encoder *self, pdu_t *pdu, encoder_status_t *status);
    int (*read) (struct encoder *self, iobuf_t *iobuf, encoder_status_t *status);
    int (*buffer) (struct encoder *self, const void **buffer, size_t *buffer_size);
    int (*advance) (struct encoder *self, size_t n, encoder_status_t *status);
    encoder_status_t (*status) (struct encoder *self);
    void (*destroy) (struct encoder **self_p);
};

struct encoder {
    struct encoder_ops ops;
};

typedef struct encoder encoder_t;

inline int
encoder_encode (encoder_t *self, pdu_t *pdu, encoder_status_t *status)
{
    return self->ops.encode (self, pdu, status);
}

inline int
encoder_read (encoder_t *self, iobuf_t *iobuf, encoder_status_t *status)
{
    return self->ops.read (self, iobuf, status);
}

inline int
encoder_buffer (encoder_t *self, const void **buffer, size_t *buffer_size)
{
    return self->ops.buffer (self, buffer, buffer_size);
}

inline int
encoder_advance (encoder_t *self, size_t n, encoder_status_t *status)
{
    return self->ops.advance (self, n, status);
}

inline encoder_status_t
encoder_status (encoder_t *self)
{
    return self->ops.status (self);
}

void
    encoder_destroy (encoder_t **self_p);

#endif
