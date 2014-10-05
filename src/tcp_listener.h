//  TCP listener class

#ifndef __TCP_LISTENER_H_INCLUDED__
#define __TCP_LISTENER_H_INCLUDED__

#include "mailbox.h"
#include "io_handler.h"
#include "msg_decoder.h"

typedef struct tcp_listener tcp_listener_t;

tcp_listener_t *
    tcp_listener_new (msg_decoder_constructor_t *decoder_constructor, struct mailbox *owner);

void
    tcp_listener_destroy (tcp_listener_t **self_p);

int
    tcp_listener_bind (tcp_listener_t *self, unsigned short port);

struct io_handler
    tcp_listener_io_handler (tcp_listener_t *self);

#endif
