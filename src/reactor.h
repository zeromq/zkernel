//  Reactor class

#ifndef __REACTOR_H_INCLUDED__
#define __REACTOR_H_INCLUDED__

#include "mailbox.h"

typedef struct reactor reactor_t;

reactor_t *
    reactor_new ();

void
    reactor_destroy (reactor_t **self_p);

mailbox_t
    reactor_mailbox ();

#endif

