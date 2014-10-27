//  I/O object interface

#ifndef __IO_HANDLER_H_INCLUDED__
#define __IO_HANDLER_H_INCLUDED__

#include <stdint.h>

struct io_object;

struct io_object_ops {
    int (*init) (struct io_object *self, int *fd, uint32_t *timer_interval);
    int (*event) (struct io_object *self, uint32_t flags, int *fd, uint32_t *timer_interval);
    int (*timeout) (struct io_object *self, int *fd, uint32_t *timer_interval);
};

struct io_object {
    void *io_handle;
    struct io_object_ops ops;
};

typedef struct io_object io_object_t;

inline int
io_object_init (io_object_t *self, int *fd, uint32_t *timer_interval)
{
    return self->ops.init (self, fd, timer_interval);
}

inline int
io_object_event (io_object_t *self, uint32_t flags, int *fd, uint32_t *timer_interval)
{
    return self->ops.event (self, flags, fd, timer_interval);
}

inline int
io_object_timeout (io_object_t *self, int *fd, uint32_t *timer_interval)
{
    return self->ops.timeout (self, fd, timer_interval);
}

#endif
