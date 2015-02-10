//  Frame class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __PDU_H_INCLUDED__
#define __PDU_H_INCLUDED__

#include <stdint.h>

#include "msg.h"

struct io_object;

struct pdu {
    msg_t base;
    struct io_object *io_object;
    size_t pdu_size;
    uint8_t pdu_data [64];
};

typedef struct pdu pdu_t;

pdu_t *
    pdu_new ();

pdu_t *
    pdu_new_with_size (size_t pdu_size);

void
    pdu_destroy (pdu_t **self_p);

#endif
