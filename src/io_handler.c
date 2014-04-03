#include "io_handler.h"

extern inline void
io_handler_error (io_handler_t *self);

extern inline int
io_handler_event (io_handler_t *self, int input_flag, int output_flag);
