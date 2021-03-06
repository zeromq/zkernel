// Socket class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __SOCKET_H_INCLUDED__
#define __SOCKET_H_INCLUDED__

#include "msg.h"
#include "reactor.h"
#include "io_object.h"
#include "protocol_engine.h"

typedef struct socket socket_t;

struct dispatcher;
struct proxy;

socket_t *
    socket_new (struct dispatcher *dispatcher, reactor_t *reactor);

void
    socket_destroy (socket_t **self_p);

struct proxy *
    socket_proxy (socket_t *self);

int
    socket_listen (socket_t *self, io_object_t *listener);

int
    socket_connect (socket_t *self, io_object_t *connector);

void
    socket_send_msg (socket_t *self, msg_t *msg);

/*
int
    socket_send (socket_t *self, const char *data, size_t size);
    */

void
    socket_noop (socket_t *self);

#endif
