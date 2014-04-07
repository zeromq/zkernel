//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mailbox.h"
#include "reactor.h"
#include "socket.h"
#include "msg.h"

int main()
{
    reactor_t *reactor = reactor_new ();
    assert (reactor);

    mailbox_t mbox = reactor_mailbox (reactor);

    socket_t *socket = socket_new (reactor);
    assert (socket);

    const int rc = socket_bind (socket, 2226);
    assert (rc != -1);

    for (int i = 0; i < 10; i++) {
        struct msg_t *msg = msg_new (0);
        mailbox_enqueue (&mbox, msg);
        printf ("press any key\n");
        getchar ();
    }
    socket_destroy (&socket);
    reactor_destroy (&reactor);
    return 0;
}
