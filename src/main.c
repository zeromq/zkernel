//

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dispatcher.h"
#include "reactor.h"
#include "socket.h"
#include "msg.h"
#include "tcp_listener.h"
#include "zmtp_handshake.h"

int main()
{
    reactor_t *reactor = reactor_new ();
    assert (reactor);

    dispatcher_t *dispatcher = dispatcher_new ();
    assert (dispatcher);

    socket_t *socket = socket_new (dispatcher, reactor);
    assert (socket);

    tcp_listener_t *listener =
        tcp_listener_new (zmtp_handshake_new_protocol_engine, socket);
    assert (listener);

    int rc = tcp_listener_bind (listener, 5566);
    assert (rc == 0);

    rc = socket_listen (socket, (io_object_t *) listener);
    assert (rc != -1);

    for (int i = 0; i < 10; i++) {
        struct msg_t *msg = msg_new (0);
        reactor_send (reactor, msg);
        printf ("press any key\n");
        getchar ();
    }
    socket_destroy (&socket);
    reactor_destroy (&reactor);
    return 0;
}
