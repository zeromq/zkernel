//  Session base class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stddef.h>

#include "session.h"

int
session_set_socket_id (session_t *self, const char *socket_id)
{
    assert (self);

    if (self->ops.set_socket_id)
        return self->ops.set_socket_id (self, socket_id);
    else
        return 0;
}

void
session_set_stream_id (session_t *self, unsigned long stream_id)
{
    assert (self);
    self->ops.set_stream_id (self, stream_id);
}

void
session_destroy (session_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        session_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
