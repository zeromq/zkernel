//  I/O buffer class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __IO_BUFFER_H_INCLUDED__
#define __IO_BUFFER_H_INCLUDED__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

struct iobuf {
    uint8_t *base;
    size_t size;
    uint8_t *r;
    uint8_t *w;
};

typedef struct iobuf iobuf_t;

iobuf_t *
    iobuf_new (size_t size);

void
    iobuf_destroy (iobuf_t **self_p);

inline void
iobuf_reset (iobuf_t *self)
{
    self->r = self->w = self->base;
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

ssize_t
    iobuf_send (iobuf_t *self, int fd);

ssize_t
    iobuf_recv (iobuf_t *self, int fd);

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

inline size_t
iobuf_write (iobuf_t *self, const void *ptr, size_t n)
{
    const size_t space = iobuf_space (self);
    if (n > space)
        n = space;
    memcpy (self->w, ptr, n);
    self->w += n;
    return n;
}

inline size_t
iobuf_write_byte (iobuf_t *self, uint8_t byte)
{
    if (iobuf_space (self) == 0)
        return 0;
    else {
        *self->w++ = byte;
        return 1;
    }
}

inline void
iobuf_put (iobuf_t *self, size_t length)
{
    self->w += length;
}

inline void
iobuf_drop (iobuf_t *self, size_t length)
{
    self->r += length;
}

inline size_t
iobuf_copy (iobuf_t *dst, iobuf_t *src, size_t n)
{
    const size_t available = iobuf_available (src);
    if (n > available)
        n = available;
    const size_t copied = iobuf_write (dst, src->r, n);
    iobuf_drop (src, copied);
    return copied;
}

inline size_t
iobuf_copy_all (iobuf_t *dst, iobuf_t *src)
{
    const size_t copied = iobuf_write (dst, src->r, iobuf_available (src));
    iobuf_drop (src, copied);
    return copied;
}

#endif
