// Socket class

#ifndef __SOCKET_H_INCLUDED__
#define __SOCKET_H_INCLUDED__

#include "protocol.h"

typedef struct socket socket_t;

socket_t *
    socket_new (reactor_t *reactor);

void
    socket_destroy (socket_t **self_p);

int
    socket_bind (socket_t *self, unsigned short port, protocol_t *protocol);

int
    socket_connect (socket_t *self, unsigned short port, protocol_t *protocol);

int
    socket_send (socket_t *self, const char *data, size_t size);

void
    socket_noop (socket_t *self);

#endif
