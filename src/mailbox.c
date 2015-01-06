//  Mailbox interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include "mailbox.h"

int
mailbox_enqueue (mailbox_t *self, struct msg_t *msg)
{
    assert (self);
    return self->ftab.enqueue (self->object, msg);
}
