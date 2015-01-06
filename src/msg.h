//  Message class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __MSG_H_INCLUDED__
#define __MSG_H_INCLUDED__

#include "mailbox.h"

struct io_object;

struct msg_t {
    int msg_type;
    mailbox_ifc_t reply_to;
    struct msg_t *next;
    struct io_object *io_object;
    int event_mask;
};

typedef struct msg_t msg_t;

msg_t *
    msg_new (int cmd);

void
    msg_destroy (msg_t **self_p);

struct kill_cmd {
    struct msg_t base;
};

typedef struct kill_cmd kill_cmd_t;

kill_cmd_t *
    kill_cmd_new ();

struct session_event {
    struct msg_t base;
    void *session;
};

typedef struct session_event session_event_t;

session_event_t *
    session_event_new ();

struct session_closed_ev {
    struct msg_t base;
    void *ptr;
};

typedef struct session_closed_ev session_closed_ev_t;

session_closed_ev_t *
    session_closed_ev_new ();

struct ready_to_send_ev {
    struct msg_t base;
    void *ptr;
};

typedef struct ready_to_send_ev ready_to_send_ev_t;

ready_to_send_ev_t *
    ready_to_send_ev_new ();

#endif
