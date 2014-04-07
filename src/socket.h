// Socket class

#ifndef __SOCKET_H_INCLUDED__
#define __SOCKET_H_INCLUDED__

typedef struct socket socket_t;

socket_t *
    socket_new (reactor_t *reactor);

void
    socket_destroy (socket_t **self_p);

int
    socket_bind (socket_t *self, unsigned short port);

void
    socket_noop (socket_t *self);

#endif
