//  I/O handler interface

#ifndef __IO_HANDLER_H_INCLUDED__
#define __IO_HANDLER_H_INCLUDED__

struct io_handler_ops {
    void (*error) (void *handler);
    int (*event) (void *handler, int input_flag, int output_flag);
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
io_handler_event (io_handler_t *self, int input_flag, int output_flag)
{
    return self->ops->event (self->object, input_flag, output_flag);
}

#endif
