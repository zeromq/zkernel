//  Frame class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdlib.h>
#include <assert.h>

#include "zkernel.h"
#include "pdu.h"

pdu_t *
pdu_new ()
{
    pdu_t *pdu = (pdu_t *) malloc (sizeof *pdu);
    if (pdu) {
        pdu->base = (msg_t) { .msg_type = ZKERNEL_MSG_TYPE_PDU };
        pdu->pdu_size = 0;
    }
    return pdu;
}

pdu_t *
pdu_new_with_size (size_t pdu_size)
{
    pdu_t *pdu = (pdu_t *) malloc (sizeof *pdu);
    if (pdu) {
        pdu->base = (msg_t) { .msg_type = ZKERNEL_MSG_TYPE_PDU };
        pdu->pdu_size = pdu_size;
    }
    return pdu;
}

void
pdu_destroy (pdu_t **self_p)
{
    assert (self_p);
    if (*self_p) {
          pdu_t *self = *self_p;
          free (self);
          *self_p = NULL;
    }
}
