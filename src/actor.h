//  Actor interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ACTOR_H_INCLUDED__
#define __ACTOR_H_INCLUDED__

typedef struct actor actor_t;

struct msg_t;

struct actor_ftab {
    int (*send)(void *, struct msg_t *msg);
};

struct actor {
    void *object;
    struct actor_ftab ftab;
};

typedef struct actor actor_ifc_t;

int
    actor_send (actor_t *self, struct msg_t *msg);

#endif
