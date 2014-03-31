//

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#include "mailbox.h"
#include "atomic.h"
#include "reactor.h"

struct reactor {
    int event_fd;
    void *lifo;
    pthread_t thread_handle;
};

static void *
    s_loop (void *udata);

static int
    s_send_msg (void *self_, struct msg_t *msg);

reactor_t *
reactor_new ()
{
    const int event_fd = eventfd (0, 0);
    if (event_fd == -1)
        return NULL;
    reactor_t *self = malloc (sizeof *self);
    if (!self) {
        close (event_fd);
        return NULL;
    }
    *self = (reactor_t) { .event_fd = event_fd };

    //  Create and start I/O thread
    const int rc = pthread_create (&self->thread_handle, NULL, s_loop, self);
    if (rc) {
        close (event_fd);
        free (self);
        return NULL;
    }
    return self;
}

void
reactor_destroy (reactor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        reactor_t *self = *self_p;
        struct msg_t *msg = malloc (sizeof *msg);
        assert (msg);
        *msg = (struct msg_t) { .cmd = 1 };
        s_send_msg (self, msg);
        pthread_join (self->thread_handle, NULL);
        *self_p = NULL;
    }
}

mailbox_t
reactor_mailbox (reactor_t *self)
{
    assert (self);
    return (mailbox_t) {
        .object = self,
        .ftab = (struct mailbox_ftab) { .enqueue = s_send_msg }
    };
}

static void *
s_loop (void *udata)
{
    reactor_t *self = (reactor_t *) udata;
    assert (self);

    int stop = 0;
    while (!stop) {
        struct pollfd pollfd = { .fd = self->event_fd, .events = POLLIN };
        int rc = poll (&pollfd, 1, -1);
        if (rc == -1)
            assert (errno == EINTR);
        assert (rc > 0);
        printf ("eventfd event\n");
        struct msg_t *tail = (struct msg_t *) atomic_ptr_swap (&self->lifo, NULL);
        uint64_t x;
        rc = read (self->event_fd, &x, sizeof x);
        assert (rc == sizeof x);
        while (tail) {
            if (tail->cmd == 1)
                stop = 1;
            struct msg_t *msg = tail;
            tail = tail->next;
            free (msg);
        }
    }

    return NULL;
}

static int
s_send_msg (void *self_, struct msg_t *msg)
{
    reactor_t *self = (reactor_t *) self_;
    void *tail = atomic_ptr_get (&self->lifo);
    atomic_ptr_set ((void **) &msg->next, tail);
    void *prev = atomic_ptr_cas (&self->lifo, tail, msg);
    while (prev != tail) {
        tail = prev;
        atomic_ptr_set ((void **) &msg->next, tail);
        prev = atomic_ptr_cas (&self->lifo, tail, &msg);
    }
    //  Wake up I/O thread if necessary
    if (!prev) {
        uint64_t v = 1;
        const int rc = write (self->event_fd, &v, sizeof v);
        assert (rc == sizeof v);
    }
    return 0;
}
