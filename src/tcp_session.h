//  TCP session class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __TCP_SESSION_H_INCLUDED__
#define __TCP_SESSION_H_INCLUDED__

#include "mailbox.h"
#include "protocol_engine.h"

typedef struct tcp_session tcp_session_t;

tcp_session_t *
    tcp_session_new (int fd, protocol_engine_t *protocol_engine, mailbox_t *owner);

void
    tcp_session_destroy (tcp_session_t **self_p);

int
    tcp_session_send (tcp_session_t *self, const char *data, size_t size);

#endif
