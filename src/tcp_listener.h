//  TCP listener class

#ifndef __TCP_LISTENER_H_INCLUDED__
#define __TCP_LISTENER_H_INCLUDED__

#include "io_handler.h"

typedef struct tcp_listener tcp_listener_t;

tcp_listener_t *
    tcp_listener_new ();

void
    tcp_listener_destroy (tcp_listener_t **self_p);

int
    tcp_listener_bind (tcp_listener_t *self, unsigned short port);

int
    tcp_listener_fd (tcp_listener_t *self);

struct io_handler
    tcp_listener_io_handler (tcp_listener_t *self);

#endif
