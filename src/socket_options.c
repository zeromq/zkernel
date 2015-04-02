//  Socket option class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "socket_options.h"

struct socket_options {
    char *socket_id;
};

socket_options_t *
socket_options_new ()
{
    socket_options_t *self = malloc (sizeof *self);
    if (self) {
        *self = (socket_options_t) {};
    }

    return self;
}

const char *
socket_options_socket_id (socket_options_t *self)
{
    assert (self);

    return self->socket_id ? self->socket_id : "";
}

int
socket_options_set_socket_id (socket_options_t *self, const char *socket_id)
{
    assert (self);

    if (socket_id == NULL || strlen (socket_id) == 0) {
        free (self->socket_id);
        self->socket_id = NULL;
    }
    else {
        char *s = malloc (strlen (socket_id) + 1);
        if (s == NULL)
            return -1;
        strcpy (s, socket_id);
        free (self->socket_id);
        self->socket_id = s;
    }

    return 0;
}
