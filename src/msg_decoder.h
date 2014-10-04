//  Message decoder interface

#ifndef __I_MSG_DECODER_H_INCLUDED__
#define __I_MSG_DECODER_H_INCLUDED__

struct msg_decoder_ops {
    void (*buffer)(void *self, void **ptr, size_t *n);
    void (*data_ready)(void *self, size_t n);
    int (*decode)(void *self);
};

struct msg_decoder {
    void *object;
    struct msg_decoder_ops ops;
};

typedef struct msg_decoder msg_decoder_t;

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

#endif
