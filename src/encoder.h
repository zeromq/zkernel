// Encoder class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ENCODER_H_INCLUDED__
#define __ENCODER_H_INCLUDED__

#include <stdbool.h>

#include "frame.h"
#include "iobuf.h"

struct encoder_info {
    bool ready;
    size_t dba_size;
};

typedef struct encoder_info encoder_info_t;

struct encoder;

struct encoder_ops {
    void (*info ) (struct encoder *self, encoder_info_t *info);
    int (*encode) (struct encoder *self, frame_t *frame, encoder_info_t *info);
    int (*read) (struct encoder *self, iobuf_t *iobuf, encoder_info_t *info);
    const void *(*buffer) (struct encoder *self);
    int (*advance) (struct encoder *self, size_t n, encoder_info_t *info);
    void (*destroy) (struct encoder **self_p);
};

struct encoder {
    struct encoder_ops ops;
};

typedef struct encoder encoder_t;

typedef encoder_t *encoder_constructor_t ();

inline void
encoder_info (encoder_t *self, encoder_info_t *info)
{
    self->ops.info (self, info);
}

inline int
encoder_encode (encoder_t *self, frame_t *frame, encoder_info_t *info)
{
    return self->ops.encode (self, frame, info);
}

inline int
encoder_read (encoder_t *self, iobuf_t *iobuf, encoder_info_t *info)
{
    return self->ops.read (self, iobuf, info);
}

inline const void *
encoder_buffer (encoder_t *self)
{
    return self->ops.buffer (self);
}

inline int
encoder_advance (encoder_t *self, size_t n, encoder_info_t *info)
{
    return self->ops.advance (self, n, info);
}

void
    encoder_destroy (encoder_t **self_p);

#endif
