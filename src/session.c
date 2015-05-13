//  Session base class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stddef.h>

#include "session.h"

void
session_destroy (session_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        session_t *self = *self_p;
        self->ops.destroy (self_p);
    }
}
