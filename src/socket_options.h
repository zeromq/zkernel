//  Socket option class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __SOCKET_OPTIONS_H_INCLUDED__
#define __SOCKET_OPTIONS_H_INCLUDED__

typedef struct socket_options socket_options_t;

void
    socket_options_destroy (socket_options_t **self_p);

const char *
    socket_options_socket_id (socket_options_t *self);

int
    socket_options_set_socket_id (
        socket_options_t *self, const char *socket_id);

#endif
