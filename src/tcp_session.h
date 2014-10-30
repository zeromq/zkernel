//  TCP session class

#ifndef __TCP_SESSION_H_INCLUDED__
#define __TCP_SESSION_H_INCLUDED__

#include "mailbox.h"
#include "io_object.h"
#include "encoder.h"
#include "decoder.h"

typedef struct tcp_session tcp_session_t;

tcp_session_t *
    tcp_session_new (int fd, encoder_constructor_t *encoder_constructor,
        decoder_constructor_t *decoder_constructor, mailbox_t *owner);

void
    tcp_session_destroy (tcp_session_t **self_p);

int
    tcp_session_send (tcp_session_t *self, const char *data, size_t size);

#endif
