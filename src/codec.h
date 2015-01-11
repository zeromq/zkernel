//  Codec class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __CODEC_H_INCLUDED__
#define __CODEC_H_INCLUDED__

#include <stdint.h>

#include "iobuf.h"
#include "frame.h"

#define ZKERNEL_CODEC_ENCODER_READY     0x01
#define ZKERNEL_CODEC_READ_OK           0x02
#define ZKERNEL_CODEC_DECODER_READY     0x04
#define ZKERNEL_CODEC_WRITE_OK          0x08
#define ZKERNEL_CODEC_ERROR             0x10

struct codec;

struct codec_ops {
    int (*init) (struct codec *self, uint32_t *status);
    int (*encode) (struct codec *self, frame_t *frame, uint32_t *status);
    int (*read) (struct codec *self, iobuf_t *iobuf, uint32_t *status);
    int (*read_buffer) (struct codec *self, const void **buffer, size_t *buffer_size);
    int (*read_advance) (struct codec *self, size_t n, uint32_t *status);
    frame_t *(*decode) (struct codec *self, uint32_t *status);
    int (*write) (struct codec *self, iobuf_t *iobuf, uint32_t *status);
    int (*write_buffer) (struct codec *self, void **buffer, size_t *buffer_size);
    int (*write_advance) (struct codec *self, size_t n, uint32_t *status);
    void (*destroy) (struct codec **self_p);
};

struct codec {
    struct codec_ops ops;
};

typedef struct codec codec_t;

typedef codec_t *(codec_constructor_t) ();

inline int
codec_init (codec_t *self, uint32_t *status)
{
    return self->ops.init (self, status);
}

inline int
codec_encode (codec_t *self, frame_t *frame, uint32_t *status)
{
    return self->ops.encode (self, frame, status);
}

inline int
codec_read (codec_t *self, iobuf_t *iobuf, uint32_t *status)
{
    return self->ops.read (self, iobuf, status);
}

inline int
codec_read_buffer (codec_t *self, const void **buffer, size_t *buffer_size)
{
    return self->ops.read_buffer (self, buffer, buffer_size);
}

inline int
codec_read_advance (codec_t *self, size_t n, uint32_t *status)
{
    return self->ops.read_advance (self, n, status);
}

inline frame_t *
codec_decode (codec_t *self, uint32_t *status)
{
    return self->ops.decode (self, status);
}

inline int
codec_write (codec_t *self, iobuf_t *iobuf, uint32_t *status)
{
    return self->ops.write (self, iobuf, status);
}

inline int
codec_write_buffer (codec_t *self, void **buffer, size_t *buffer_size)
{
    return self->ops.write_buffer (self, buffer, buffer_size);
}

inline int
codec_write_advance (codec_t *self, size_t n, uint32_t *status)
{
    return self->ops.write_advance (self, n, status);
}

void
    codec_destroy (codec_t **self_p);

#endif
