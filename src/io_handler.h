//  I/O handler interface

#ifndef __IO_HANDLER_H_INCLUDED__
#define __IO_HANDLER_H_INCLUDED__

#include <stdint.h>

struct io_handler_ops {
    int (*init) (void *handler, int *fd, uint32_t *timer_interval);
    int (*event) (void *handler, uint32_t flags, uint32_t *timer_interval);
    void (*error) (void *handler);
};

struct io_handler {
    void *object;
    struct io_handler_ops *ops;
};

typedef struct io_handler io_handler_t;

inline void
io_handler_error (io_handler_t *self)
{
    return self->ops->error (self->object);
}

inline int
io_handler_init (io_handler_t *self, int *fd, uint32_t *timer_interval)
{
    return self->ops->init (self->object, fd, timer_interval);
}

inline int
io_handler_event (io_handler_t *self, uint32_t flags, uint32_t *timer_interval)
{
    return self->ops->event (self->object, flags, timer_interval);
}

#endif
