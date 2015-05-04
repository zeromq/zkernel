//  Message class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdlib.h>
#include <assert.h>

#include "msg.h"
#include "zkernel.h"

msg_t *
msg_new (int msg_type)
{
    msg_t *self = (msg_t *) malloc (sizeof *self);
    if (self)
        *self = (struct msg_t) { .msg_type = msg_type };
    return self;
}

void
msg_destroy (msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        msg_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}

kill_cmd_t *
kill_cmd_new ()
{
    kill_cmd_t *cmd = (kill_cmd_t *) malloc (sizeof *cmd);
    if (cmd != NULL)
        *cmd = (kill_cmd_t) { .base.msg_type = ZKERNEL_KILL };
    return cmd;
}
