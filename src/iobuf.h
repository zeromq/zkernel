//  I/O buffer class

#ifndef __IO_BUFFER_H_INCLUDED__
#define __IO_BUFFER_H_INCLUDED__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct iobuf {
    uint8_t *base;
    size_t size;
    uint8_t *r;
    uint8_t *w;
};

typedef struct iobuf iobuf_t;

iobuf_t *
    iobuf_new (uint8_t *base, size_t size);

void
    iobuf_destroy (iobuf_t **self_p);

inline void
iobuf_init (iobuf_t *self, uint8_t *base, size_t size)
{
    *self = (iobuf_t) { .base = base, .size = size, .r = base, .w = base };
}

inline bool
iobuf_is_empty (iobuf_t *self)
{
    return self->r == self->w;
}

inline size_t
iobuf_available (iobuf_t *self)
{
    return (size_t) (self->w - self->r);
}

inline size_t
iobuf_space (iobuf_t *self)
{
    return (size_t) (self->base + self->size - self->w);
}

inline size_t
iobuf_read (iobuf_t *self, void *ptr, size_t n)
{
    const size_t available = iobuf_available (self);
    if (n > available)
        n = available;
    memcpy (ptr, self->r, n);
    self->r += n;
    return n;
}

inline void
iobuf_put (iobuf_t *self, size_t length)
{
    self->w += length;
}

#endif
