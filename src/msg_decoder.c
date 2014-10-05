#include "msg_decoder.h"

extern inline void
msg_decoder_buffer (msg_decoder_t *self, void **ptr, size_t *n);

extern inline void
msg_decoder_data_ready (msg_decoder_t *self, size_t n);

extern inline int
msg_decoder_decode (msg_decoder_t *self);

extern inline void
msg_decoder_destroy (msg_decoder_t **self_p);
