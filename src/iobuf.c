// I/O buffer class

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "iobuf.h"

iobuf_t *
iobuf_new (uint8_t *data, size_t size)
{
    iobuf_t *self = (iobuf_t *) malloc (sizeof *self);
    if (self)
        iobuf_init (self, data, size);
    return self;
}

void
iobuf_destroy (iobuf_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        iobuf_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}

extern inline void
iobuf_init (iobuf_t *self, uint8_t *base, size_t size);

extern inline void
iobuf_reset (iobuf_t *self);

extern inline bool
iobuf_is_empty (iobuf_t *self);

extern inline size_t
iobuf_available (iobuf_t *self);

extern inline size_t
iobuf_space (iobuf_t *self);

ssize_t
iobuf_send (iobuf_t *self, int fd)
{
    const ssize_t rc = send (fd, self->r, iobuf_available (self), 0);
    if (rc > 0)
        self->r += rc;
    return rc;
}

ssize_t
iobuf_recv (iobuf_t *self, int fd)
{
    const int rc = recv (fd, self->base, iobuf_space (self), 0);
    if (rc > 0)
        self->w += rc;
    return rc;
}

extern inline size_t
iobuf_read (iobuf_t *self, void *ptr, size_t n);

extern inline size_t
iobuf_write (iobuf_t *self, const void *ptr, size_t n);

extern inline void
iobuf_put (iobuf_t *self, size_t length);
