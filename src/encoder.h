// Encoder class

#ifndef __ENCODER_H_INCLUDED__
#define __ENCODER_H_INCLUDED__

#include <stdbool.h>

#include "frame.h"
#include "iobuf.h"

struct encoder_info {
    bool done;
    size_t dba_size;
};

typedef struct encoder_info encoder_info_t;

struct encoder_ops {
    int (*encode) (void *self, frame_t *frame, encoder_info_t *info);
    int (*read) (void *self, iobuf_t *iobuf, encoder_info_t *info);
    int (*buffer) (void *self, iobuf_t *iobuf, encoder_info_t *info);
    void (*destroy) (void **self_p);
};

struct encoder {
    void *object;
    struct encoder_ops ops;
};

typedef struct encoder encoder_t;

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

inline int
encoder_buffer (encoder_t *self, iobuf_t *iobuf, encoder_info_t *info)
{
    return self->ops.buffer (self->object, iobuf, info);
}

void
    encoder_destroy (encoder_t **self_p);

#endif
