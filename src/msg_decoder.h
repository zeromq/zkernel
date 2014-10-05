//  Message decoder interface

#ifndef __MSG_DECODER_H_INCLUDED__
#define __MSG_DECODER_H_INCLUDED__

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>

struct msg_decoder_ops {
    void (*buffer)(void *self, void **ptr, size_t *n);
    void (*data_ready)(void *self, size_t n);
    int (*decode)(void *self);
    void (*destroy) (void **self_p);
};

struct msg_decoder {
    void *object;
    struct msg_decoder_ops ops;
};

typedef struct msg_decoder msg_decoder_t;

typedef msg_decoder_t *msg_decoder_constructor_t ();

inline void
msg_decoder_buffer (msg_decoder_t *self, void **ptr, size_t *n)
{
    self->ops.buffer (self->object, ptr, n);
}

inline void
msg_decoder_data_ready (msg_decoder_t *self, size_t n)
{
    return self->ops.data_ready (self->object, n);
}

inline int
msg_decoder_decode (msg_decoder_t *self)
{
    return self->ops.decode (self->object);
}

inline void
msg_decoder_destroy (msg_decoder_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        msg_decoder_t *self = *self_p;
        self->ops.destroy (&self->object);
        free (self);
        *self_p = NULL;
    }
}

#endif
