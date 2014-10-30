// Encoder class

#ifndef __ENCODER_H_INCLUDED__
#define __ENCODER_H_INCLUDED__

#include <stdint.h>
#include <stdbool.h>

#include "frame.h"
#include "iobuf.h"

struct encoder_info {
    bool ready;
    size_t dba_size;
};

typedef struct encoder_info encoder_info_t;

struct encoder_ops {
    int (*encode) (void *self, frame_t *frame, encoder_info_t *info);
    int (*read) (void *self, iobuf_t *iobuf, encoder_info_t *info);
    uint8_t *(*buffer) (void *self);
    int (*advance) (void *self, size_t n, encoder_info_t *info);
    void (*destroy) (void **self_p);
};

struct encoder {
    void *object;
    struct encoder_ops ops;
};

typedef struct encoder encoder_t;

typedef encoder_t *encoder_constructor_t ();

inline int
encoder_encode (encoder_t *self, frame_t *frame, encoder_info_t *info)
{
    return self->ops.encode (self->object, frame, info);
}

inline int
encoder_read (encoder_t *self, iobuf_t *iobuf, encoder_info_t *info)
{
    return self->ops.read (self->object, iobuf, info);
}

inline uint8_t *
encoder_buffer (encoder_t *self)
{
    return self->ops.buffer (self->object);
}

inline int
encoder_advance (encoder_t *self, size_t n, encoder_info_t *info)
{
    return self->ops.advance (self, n, info);
}

void
    encoder_destroy (encoder_t **self_p);

#endif
