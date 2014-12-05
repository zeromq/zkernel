//  Frame class

#include <stdlib.h>
#include <assert.h>

#include "zkernel.h"
#include "frame.h"

frame_t *
frame_new ()
{
    frame_t *frame = (frame_t *) malloc (sizeof *frame);
    if (frame) {
        frame->base = (msg_t) { .msg_type = ZKERNEL_MSG_TYPE_FRAME };
        frame->frame_size = 0;
    }
    return frame;
}

frame_t *
frame_new_with_size (size_t frame_size)
{
    frame_t *frame = (frame_t *) malloc (sizeof *frame);
    if (frame) {
        frame->base = (msg_t) { .msg_type = ZKERNEL_MSG_TYPE_FRAME };
        frame->frame_size = frame_size;
    }
    return frame;
}

void
frame_destroy (frame_t **self_p)
{
    assert (self_p);
    if (*self_p) {
          frame_t *self = *self_p;
          free (self);
          *self_p = NULL;
    }
}
