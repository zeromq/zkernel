//  Message class

#ifndef __MSG_H_INCLUDED__
#define __MSG_H_INCLUDED__

#include "mailbox.h"
#include "io_handler.h"

struct msg_t {
    int cmd;
    mailbox_ifc_t reply_to;
    struct msg_t *next;
    int fd;
    io_handler_t handler;
    void *ptr;
};

typedef struct msg_t msg_t;

msg_t *
    msg_new (int cmd);

void
    msg_destroy (msg_t **self_p);

#endif

