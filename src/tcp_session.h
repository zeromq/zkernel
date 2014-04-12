//  TCP session class

#ifndef __TCP_SESSION_H_INCLUDED__
#define __TCP_SESSION_H_INCLUDED__

#include "mailbox.h"
#include "io_handler.h"

typedef struct tcp_session tcp_session_t;

tcp_session_t *
    tcp_session_new (int fd, mailbox_t *owner);

void
    tcp_session_destroy (tcp_session_t **self_p);

struct io_handler
    tcp_session_io_handler (tcp_session_t *self);

int
    tcp_session_fd (tcp_session_t *self);

#endif
