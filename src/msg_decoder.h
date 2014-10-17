//  Message decoder interface

#ifndef __MSG_DECODER_H_INCLUDED__
#define __MSG_DECODER_H_INCLUDED__

#include <stdbool.h>
#include <stdint.h>

#include "iobuf.h"
#include "frame.h"

struct msg_decoder_info {
    bool ready;
    size_t dba_size;
};

typedef struct msg_decoder_info msg_decoder_info_t;

struct msg_decoder_ops {
    int (*write) (void *self, iobuf_t *iobuf, msg_decoder_info_t *info);
    uint8_t *(*buffer) (void *self);
    int (*advance) (void *self, size_t n, msg_decoder_info_t *info);
    frame_t *(*decode) (void *self, msg_decoder_info_t *info);
    void (*destroy) (void **self_p);
};

struct msg_decoder {
    void *object;
    struct msg_decoder_ops ops;
};

typedef struct msg_decoder msg_decoder_t;

typedef msg_decoder_t *msg_decoder_constructor_t ();

inline int
msg_decoder_write (
    msg_decoder_t *self, iobuf_t *iobuf, msg_decoder_info_t *info)
{
    return self->ops.write (self, iobuf, info);
}

inline uint8_t *
msg_decoder_buffer (msg_decoder_t *self)
{
    return self->ops.buffer (self->object);
}

inline int
msg_decoder_advance (msg_decoder_t *self, size_t n, msg_decoder_info_t *info)
{
    return self->ops.advance (self, n, info);
}

inline frame_t *
msg_decoder_decode (msg_decoder_t *self, msg_decoder_info_t *info)
{
    return self->ops.decode (self->object, info);
}

void
    msg_decoder_destroy (msg_decoder_t **self_p);

#endif
