// Socket class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __SOCKET_H_INCLUDED__
#define __SOCKET_H_INCLUDED__

#include "protocol_engine.h"

typedef struct socket socket_t;

socket_t *
    socket_new (reactor_t *reactor);

void
    socket_destroy (socket_t **self_p);

int
    socket_bind (socket_t *self, unsigned short port, protocol_engine_constructor_t *protocol_engine_constructor);

int
    socket_connect (socket_t *self, unsigned short port, protocol_engine_constructor_t *protocol_engine_constructor);

int
    socket_send (socket_t *self, const char *data, size_t size);

void
    socket_noop (socket_t *self);

#endif
