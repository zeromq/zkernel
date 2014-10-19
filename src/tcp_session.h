//  TCP session class

#ifndef __TCP_SESSION_H_INCLUDED__
#define __TCP_SESSION_H_INCLUDED__

#include "mailbox.h"
#include "io_object.h"
#include "decoder.h"

typedef struct tcp_session tcp_session_t;

tcp_session_t *
    tcp_session_new (int fd, decoder_constructor_t *decoder_constructor, mailbox_t *owner);

void
    tcp_session_destroy (tcp_session_t **self_p);

io_object_t *
    tcp_session_io_object (tcp_session_t *self);

int
    tcp_session_send (tcp_session_t *self, const char *data, size_t size);

#endif
