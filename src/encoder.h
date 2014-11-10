// Encoder class

#ifndef __ENCODER_H_INCLUDED__
#define __ENCODER_H_INCLUDED__

#include <stdint.h>
#include <stdbool.h>

#include "frame.h"
#include "iobuf.h"

struct encoder;

struct encoder_ops {
    int (*encode) (struct encoder *self, frame_t *frame);
    ssize_t (*read) (struct encoder *self, iobuf_t *iobuf);
    uint8_t *(*buffer) (struct encoder *self);
    int (*advance) (struct encoder *self, size_t n);
    void (*destroy) (struct encoder **self_p);
};

struct encoder {
    bool ready;
    size_t dba_size;
    struct encoder_ops ops;
};

typedef struct encoder encoder_t;

typedef encoder_t *encoder_constructor_t ();

inline int
encoder_encode (encoder_t *self, frame_t *frame)
{
    return self->ops.encode (self, frame);
}

inline ssize_t
encoder_read (encoder_t *self, iobuf_t *iobuf)
{
    return self->ops.read (self, iobuf);
}

inline uint8_t *
encoder_buffer (encoder_t *self)
{
    return self->ops.buffer (self);
}

inline int
encoder_advance (encoder_t *self, size_t n)
{
    return self->ops.advance (self, n);
}

void
    encoder_destroy (encoder_t **self_p);

#endif
