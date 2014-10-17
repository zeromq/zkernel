//  TCP connector class

#ifndef __TCP_CONNECTOR_H_INCLUDED__
#define __TCP_CONNECTOR_H_INCLUDED__

#include "mailbox.h"
#include "io_handler.h"
#include "decoder.h"

typedef struct tcp_connector tcp_connector_t;

tcp_connector_t *
    tcp_connector_new (decoder_constructor_t *decoder_constructor, mailbox_t *owner);

void
    tcp_connector_destroy (tcp_connector_t **self_p);

int
    tcp_connector_connect (tcp_connector_t *self, unsigned short port);

int
    tcp_connector_errno (tcp_connector_t *self);

struct io_handler
    tcp_connector_io_handler (tcp_connector_t *self);

#endif
