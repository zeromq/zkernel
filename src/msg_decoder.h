//  Message decoder interface

#ifndef __MSG_DECODER_H_INCLUDED__
#define __MSG_DECODER_H_INCLUDED__

#include <stdbool.h>

#include "iobuf.h"
#include "frame.h"

struct msg_decoder_result {
    size_t dba_size;
    frame_t *frame;
};

typedef struct msg_decoder_result msg_decoder_result_t;

struct msg_decoder_ops {
    int (*buffer) (void *self, iobuf_t *iobuf);
    int (*decode) (void *self, iobuf_t *iobuf, msg_decoder_result_t *res);
    int (*error) (void *self);
    void (*destroy) (void **self_p);
};

struct msg_decoder {
    void *object;
    struct msg_decoder_ops ops;
};

typedef struct msg_decoder msg_decoder_t;

typedef msg_decoder_t *msg_decoder_constructor_t ();

inline int
msg_decoder_buffer (msg_decoder_t *self, iobuf_t *iobuf)
{
    return self->ops.buffer (self->object, iobuf);
}

inline int
msg_decoder_decode (
    msg_decoder_t *self, iobuf_t *iobuf, msg_decoder_result_t *res)
{
    return self->ops.decode (self->object, iobuf, res);
}

int
    msg_decoder_error (msg_decoder_t *self);

void
    msg_decoder_destroy (msg_decoder_t **self_p);

#endif
