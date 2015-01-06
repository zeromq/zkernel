//  TCP connector class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __TCP_CONNECTOR_H_INCLUDED__
#define __TCP_CONNECTOR_H_INCLUDED__

#include "mailbox.h"
#include "protocol.h"

typedef struct tcp_connector tcp_connector_t;

tcp_connector_t *
    tcp_connector_new (protocol_t *protocol, mailbox_t *owner);

void
    tcp_connector_destroy (tcp_connector_t **self_p);

int
    tcp_connector_connect (tcp_connector_t *self, unsigned short port);

int
    tcp_connector_errno (tcp_connector_t *self);

#endif
