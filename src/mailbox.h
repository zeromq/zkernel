//  Mailbox interface

#ifndef __MAILBOX_H_INCLUDED__
#define __MAILBOX_H_INCLUDED__

typedef struct mailbox mailbox_t;

#include "io_handler.h"

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
