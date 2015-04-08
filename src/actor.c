//  Actor interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include "actor.h"

int
actor_send (actor_t *self, struct msg_t *msg)
{
    assert (self);
    return self->ftab.send (self->object, msg);
}
