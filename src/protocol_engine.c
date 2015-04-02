//  Protocol engine class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>

#include "protocol_engine.h"

extern inline int
protocol_engine_init (protocol_engine_t *self, protocol_engine_info_t *info);

extern inline int
protocol_engine_encode (protocol_engine_t *self, pdu_t *pdu, protocol_engine_info_t *info);

extern inline int
protocol_engine_read (protocol_engine_t *self, iobuf_t *iobuf, protocol_engine_info_t *info);

extern inline int
protocol_engine_read_advance (protocol_engine_t *self, size_t n, protocol_engine_info_t *info);

extern inline pdu_t *
protocol_engine_decode (protocol_engine_t *self, protocol_engine_info_t *info);

extern inline int
protocol_engine_write (protocol_engine_t *self, iobuf_t *iobuf, protocol_engine_info_t *info);

extern inline int
protocol_engine_write_advance (protocol_engine_t *self, size_t n, protocol_engine_info_t *info);

int
protocol_engine_set_socket_id (protocol_engine_t *self, const char *socket_id)
{
    assert (self);

    if (self->ops.set_socket_id)
        return self->ops.set_socket_id (self, socket_id);
    else
        return 0;
}

int
protocol_engine_next (protocol_engine_t **self_p, protocol_engine_info_t *info)
{
    assert (self_p);
    if (*self_p) {
        protocol_engine_t *self = *self_p;
        if (self->ops.next)
            return self->ops.next (self_p, info);
        else
            return -1;
    }
    else
        return -1;
}

void
protocol_engine_destroy (protocol_engine_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        protocol_engine_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
