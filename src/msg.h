//  Message class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __MSG_H_INCLUDED__
#define __MSG_H_INCLUDED__

#include "actor.h"

struct io_object;
struct proxy;

struct msg_t {
    int msg_type;
    actor_t reply_to;
    struct msg_t *next;
    struct io_object *io_object;
    struct proxy *proxy;
    int event_mask;
    union {
        struct {
            struct session *session;
        } session;

        struct {
            struct io_object *io_object;
            actor_t reply_to;
        } start;

        struct {
            struct io_object *io_object;
        } start_ack;

        struct {
            struct io_object *io_object;
        } start_nak;

        struct {
        } stop_proxy;

        struct {
        } proxy_stopped;
    } u;
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
