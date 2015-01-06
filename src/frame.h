//  Frame class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __FRAME_H_INCLUDED__
#define __FRAME_H_INCLUDED__

#include <stdint.h>

#include "msg.h"

struct io_object;

struct frame {
    msg_t base;
    struct io_object *io_object;
    size_t frame_size;
    uint8_t frame_data [64];
};

typedef struct frame frame_t;

frame_t *
    frame_new ();

frame_t *
    frame_new_with_size (size_t frame_size);

void
    frame_destroy (frame_t **self_p);

#endif
