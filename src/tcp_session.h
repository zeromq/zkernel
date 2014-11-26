//  TCP session class

#ifndef __TCP_SESSION_H_INCLUDED__
#define __TCP_SESSION_H_INCLUDED__

#include "mailbox.h"
#include "selector.h"

typedef struct tcp_session tcp_session_t;

tcp_session_t *
    tcp_session_new (int fd, selector_t *selector, mailbox_t *owner);

void
    tcp_session_destroy (tcp_session_t **self_p);

int
    tcp_session_send (tcp_session_t *self, const char *data, size_t size);

#endif
