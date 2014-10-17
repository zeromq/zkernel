//  Message decoder interface

#ifndef __DECODER_H_INCLUDED__
#define __DECODER_H_INCLUDED__

#include <stdbool.h>
#include <stdint.h>

#include "iobuf.h"
#include "frame.h"

struct decoder_info {
    bool ready;
    size_t dba_size;
};

typedef struct decoder_info decoder_info_t;

struct decoder_ops {
    int (*write) (void *self, iobuf_t *iobuf, decoder_info_t *info);
    uint8_t *(*buffer) (void *self);
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

inline int
decoder_write (decoder_t *self, iobuf_t *iobuf, decoder_info_t *info)
{
    return self->ops.write (self, iobuf, info);
}

inline uint8_t *
decoder_buffer (decoder_t *self)
{
    return self->ops.buffer (self->object);
}

inline int
decoder_advance (decoder_t *self, size_t n, decoder_info_t *info)
{
    return self->ops.advance (self, n, info);
}

inline frame_t *
decoder_decode (decoder_t *self, decoder_info_t *info)
{
    return self->ops.decode (self->object, info);
}

void
    decoder_destroy (decoder_t **self_p);

#endif
