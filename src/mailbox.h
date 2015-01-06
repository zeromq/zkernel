//  Mailbox interface

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __MAILBOX_H_INCLUDED__
#define __MAILBOX_H_INCLUDED__

typedef struct mailbox mailbox_t;

struct msg_t;

struct mailbox_ftab {
    int (*enqueue)(void *, struct msg_t *msg);
};

struct mailbox {
    void *object;
    struct mailbox_ftab ftab;
};

typedef struct mailbox mailbox_ifc_t;

int
    mailbox_enqueue (mailbox_t *self, struct msg_t *msg);

#endif
