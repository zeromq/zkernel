//  I/O handler interface

#ifndef __IO_HANDLER_H_INCLUDED__
#define __IO_HANDLER_H_INCLUDED__

#include <stdint.h>

struct io_handler_ops {
    int (*event) (void *handler, uint32_t flags);
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
io_handler_event (io_handler_t *self, uint32_t flags)
{
    return self->ops->event (self->object, flags);
}

#endif
