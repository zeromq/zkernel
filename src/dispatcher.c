//  Message dispatcher class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include "atomic.h"
#include "msg.h"
#include "dispatcher.h"
#include "zkernel.h"

struct proxy;
void proxy_message (struct proxy *proxy, msg_t *msg);

struct dispatcher {
    int ctrl_fd;
    void *mbox;
    pthread_t thread_handle;
};

static void *
    s_loop (void *udata);

dispatcher_t *
dispatcher_new ()
{
    dispatcher_t *self = (dispatcher_t *) malloc (sizeof *self);
    if (self) {
        *self = (dispatcher_t) { .ctrl_fd = eventfd (0, 0) };
        if (self->ctrl_fd == -1) {
            free (self);
            self = NULL;
        }
        if (self) {
            const int rc =
                pthread_create (&self->thread_handle, NULL, s_loop, self);
            if (rc) {
                close (self->ctrl_fd);
                free (self);
                self = NULL;
            }
        }
    }

    return self;
}

void
dispatcher_destroy (dispatcher_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        dispatcher_t *self = *self_p;
        kill_cmd_t *cmd = kill_cmd_new ();
        assert (cmd);
        dispatcher_send (self, (msg_t *) cmd);
        pthread_join (self->thread_handle, NULL);
        close (self->ctrl_fd);
        free (self);
        *self_p = NULL;
    }
}

void
dispatcher_send (dispatcher_t *self, msg_t *msg)
{
    void *tail = atomic_ptr_get (&self->mbox);
    atomic_ptr_set ((void **) &msg->next, tail);
    void *prev = atomic_ptr_cas (&self->mbox, tail, msg);
    while (prev != tail) {
        tail = prev;
        atomic_ptr_set ((void **) &msg->next, tail);
        prev = atomic_ptr_cas (&self->mbox, tail, msg);
    }
    //  Wake up I/O thread if necessary
    if (prev == NULL) {
        const uint64_t v = 1;
        const int rc = write (self->ctrl_fd, &v, sizeof v);
        assert (rc == sizeof v);
    }
}

static void *
s_loop (void *udata)
{
    dispatcher_t *self = (dispatcher_t *) udata;
    assert (self);

    bool stop = false;

    while (!stop) {
        uint64_t x;
        const int rc = read (self->ctrl_fd, &x, sizeof x);
        assert (rc == sizeof x);

        struct msg_t *msg =
            (struct msg_t *) atomic_ptr_swap (&self->mbox, NULL);
        assert (msg);

        //  Transform LIFO to FIFO
        msg_t *prev = NULL;
        while (msg->next) {
            msg_t *next = msg->next;
            msg->next = prev;
            prev = msg;
            msg = next;
        }
        msg->next = prev;

        while (msg) {
            struct msg_t *next_msg = msg->next;
            if (msg->msg_type == ZKERNEL_KILL) {
                stop = true;
                msg_destroy (&msg);
            }
            else
            if (msg->proxy)
                proxy_message (msg->proxy, msg);
            else
                msg_destroy (&msg);
            msg = next_msg;
        }
    }

    return NULL;
}
