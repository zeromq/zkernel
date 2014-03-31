//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mailbox.h"
#include "reactor.h"

int main()
{
    reactor_t *reactor = reactor_new ();
    assert (reactor);

    mailbox_t mbox = reactor_mailbox (reactor);

    for (int i = 0; i < 10; i++) {
        struct msg_t *msg = malloc (sizeof *msg);
        mailbox_enqueue (&mbox, msg);
        printf ("press any key\n");
        getchar ();
    }
    reactor_destroy (&reactor);
    return 0;
}
