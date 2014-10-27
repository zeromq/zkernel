//  Frame class

#ifndef __FRAME_H_INCLUDED__
#define __FRAME_H_INCLUDED__

#include <stdint.h>

#include "msg.h"

struct frame {
    msg_t base;
    io_object_t *io_object;
    size_t frame_size;
    uint8_t frame_data [64];
};

typedef struct frame frame_t;

frame_t *
    frame_new ();

void
    frame_destroy (frame_t **self_p);

#endif
