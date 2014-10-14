// Encoder class

#ifndef __ENCODER_H_INCLUDED__
#define __ENCODER_H_INCLUDED__

#include "frame.h"
#include "iobuf.h"

struct encoder_ops {
    int (*frame) (void *self, frame_t **frame_p);
    int (*encode) (void *self, iobuf_t *iobuf);
    int (*error) (void *self);
    void (*destroy) (void **self_p);
};

struct encoder {
    void *object;
    struct encoder_ops ops;
};

typedef struct encoder encoder_t;

inline int
encoder_frame (encoder_t *self, frame_t **frame_p)
{
    return self->ops.frame (self->object, frame_p);
}

inline int
encoder_encode (encoder_t *self, iobuf_t *iobuf)
{
    return self->ops.encode (self->object, iobuf);
}

inline int
encoder_error (encoder_t *self)
{
    return self->ops.error (self->object);
}

void
    encoder_destroy (encoder_t **self_p);

#endif
