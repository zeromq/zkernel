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
    struct msg_t *next;
    struct proxy *proxy;
    union {
        struct {
            struct io_object *session;
            void *socket_handle;
        } session;

        struct {
            struct io_object *io_object;
            void *socket_handle;
            actor_t reply_to;
        } start_io;

        struct {
            void *socket_handle;
        } start_io_ack;

        struct {
            void *socket_handle;
        } start_io_nak;

        struct {
            unsigned long object_id;
            void *io_handle;
            actor_t reply_to;
        } stop_io;

        struct {
            unsigned long object_id;
        } stop_io_ack;

        struct {
        } session_closed;

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

#endif
