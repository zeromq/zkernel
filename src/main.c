//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mailbox.h"
#include "reactor.h"
#include "tcp_listener.h"

int main()
{
    reactor_t *reactor = reactor_new ();
    assert (reactor);

    mailbox_t mbox = reactor_mailbox (reactor);
    tcp_listener_t *listener = tcp_listener_new ();
    assert (listener);

    int rc = tcp_listener_bind (listener, 2226);
    assert (rc != -1);

    struct msg_t *msg = malloc (sizeof *msg);
    assert (msg);

    *msg = (struct msg_t) {
        .cmd = ZKERNEL_BIND,
        .fd = tcp_listener_fd (listener),
        .handler = tcp_listener_io_handler (listener)

    };
    mailbox_enqueue (&mbox, msg);

    for (int i = 0; i < 10; i++) {
        struct msg_t *msg = malloc (sizeof *msg);
        mailbox_enqueue (&mbox, msg);
        printf ("press any key\n");
        getchar ();
    }
    reactor_destroy (&reactor);
    return 0;
}
