#include "io_handler.h"

extern inline void
io_handler_error (io_handler_t *self);

extern inline int
io_handler_init (io_handler_t *self, int *fd, uint32_t *timer_interval);

extern inline int
io_handler_event (io_handler_t *self, uint32_t flags, int *fd, uint32_t *timer_interval);

extern inline int
io_handler_timeout (io_handler_t *self, int *fd, uint32_t *timer_interval);
