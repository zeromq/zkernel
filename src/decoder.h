//  Message decoder interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __DECODER_H_INCLUDED__
#define __DECODER_H_INCLUDED__

#include <stdbool.h>

#include "iobuf.h"
#include "frame.h"

struct decoder_info {
    bool ready;
    size_t dba_size;
};

typedef struct decoder_info decoder_info_t;

struct decoder_ops {
    void (*info) (void *self, decoder_info_t *info);
    int (*write) (void *self, iobuf_t *iobuf, decoder_info_t *info);
    void *(*buffer) (void *self);
    int (*advance) (void *self, size_t n, decoder_info_t *info);
    frame_t *(*decode) (void *self, decoder_info_t *info);
    void (*destroy) (void **self_p);
};

struct decoder {
    void *object;
    struct decoder_ops ops;
};

typedef struct decoder decoder_t;

typedef decoder_t *decoder_constructor_t ();

inline void
decoder_info (decoder_t *self, decoder_info_t *info)
{
    self->ops.info (self->object, info);
}

inline int
decoder_write (decoder_t *self, iobuf_t *iobuf, decoder_info_t *info)
{
    return self->ops.write (self->object, iobuf, info);
}

inline void *
decoder_buffer (decoder_t *self)
{
    return self->ops.buffer (self->object);
}

inline int
decoder_advance (decoder_t *self, size_t n, decoder_info_t *info)
{
    return self->ops.advance (self->object, n, info);
}

inline frame_t *
decoder_decode (decoder_t *self, decoder_info_t *info)
{
    return self->ops.decode (self->object, info);
}

void
    decoder_destroy (decoder_t **self_p);

#endif
