//  Reactor class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

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

