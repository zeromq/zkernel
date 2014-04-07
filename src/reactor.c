//

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <unistd.h>

#include "mailbox.h"
#include "atomic.h"
#include "reactor.h"
#include "io_handler.h"
#include "msg.h"

struct event_source {
    int fd;
    uint32_t event_mask;
    io_handler_t handler;
};

struct reactor {
    int poll_fd;
    int ctrl_fd;
    struct event_source controler;
    void *lifo;
    pthread_t thread_handle;
};

static void *
    s_loop (void *udata);

static int
    s_send_msg (void *self_, msg_t *msg);

static int
    s_register (reactor_t *self, int fd, io_handler_t *handler);

reactor_t *
reactor_new ()
{
    int poll_fd = 1, ctrl_fd = -1, rc;
    reactor_t *self = NULL;

    poll_fd = epoll_create (1);
    if (poll_fd == -1)
        goto fail;
    ctrl_fd = eventfd (0, 0);
    if (ctrl_fd == -1)
        goto fail;
    self = malloc (sizeof *self);
    if (!self)
        goto fail;

    //  Register event descriptor.
    *self = (reactor_t) {
        .poll_fd = poll_fd,
        .ctrl_fd = ctrl_fd,
        .controler = {
            .fd = ctrl_fd,
            .event_mask = EPOLLIN
        }
    };
    struct epoll_event ev = {
        .events = EPOLLIN,
        .data = &self->controler
    };
    rc = epoll_ctl (poll_fd, EPOLL_CTL_ADD, ctrl_fd, &ev);
    assert (rc == 0);

    //  Create and start I/O thread
    rc = pthread_create (&self->thread_handle, NULL, s_loop, self);
    if (rc)
        goto fail;
    return self;

fail:
    if (ctrl_fd != -1)
        close (ctrl_fd);
    if (poll_fd != -1)
        close (poll_fd);
    if (self)
        free (self);
    return NULL;
}

void
reactor_destroy (reactor_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        reactor_t *self = *self_p;
        msg_t *msg = msg_new (ZKERNEL_KILL);
        assert (msg);
        s_send_msg (self, msg);
        pthread_join (self->thread_handle, NULL);
        close (self->poll_fd);
        close (self->ctrl_fd);
        free (self);
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
#define MAX_EVENTS 32
    struct epoll_event events [MAX_EVENTS];

    while (!stop) {
        const int nfds = epoll_wait (self->poll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            assert (errno == EINTR);
            continue;
        }
        for (int i = 0; i < nfds; i++) {
            const uint32_t what = events [i].events;
            struct event_source *ev_src =
                (struct event_source *) events [i].data.ptr;
            if (ev_src->fd == self->ctrl_fd) {
                printf ("eventfd event\n");
                struct msg_t *msg =
                    (struct msg_t *) atomic_ptr_swap (&self->lifo, NULL);
                assert (msg);
                uint64_t x;
                const int rc = read (self->ctrl_fd, &x, sizeof x);
                assert (rc == sizeof x);
                while (msg) {
                    struct msg_t *next_msg = msg->next;
                    if (msg->cmd == ZKERNEL_KILL) {
                        msg_destroy (&msg);
                        stop = 1;
                    }
                    else
                    if (msg->cmd == ZKERNEL_REGISTER) {
                        s_register (self, msg->fd, &msg->handler);
                        //  Echo request (to test reactor->socket communication path)
                        mailbox_enqueue (&msg->reply_to, msg);
                    }
                    else
                        msg_destroy (&msg);
                    msg = next_msg;
                }
            }
            else
            if ((what & (EPOLLERR | EPOLLHUP)) != 0) {
                io_handler_error (&ev_src->handler);
                struct epoll_event ev;
                const int rc =
                    epoll_ctl (self->poll_fd, EPOLL_CTL_DEL, ev_src->fd, &ev);
                assert (rc == 0);
                ev_src->fd = -1;
            }
            else {
                uint32_t flags = 0;
#define ZKERNEL_INPUT_READY     0x01
#define ZKERNEL_OUTPUT_READY    0x02
                if ((what & EPOLLIN) == EPOLLIN) {
                    flags |= ZKERNEL_INPUT_READY;
                    ev_src->event_mask &= ~EPOLLIN;
                }
                if ((what & EPOLLOUT) == EPOLLOUT) {
                    flags |= ZKERNEL_OUTPUT_READY;
                    ev_src->event_mask &= ~EPOLLOUT;
                }
                const int rc = io_handler_event (&ev_src->handler, flags);
#define ZKERNEL_POLLIN 1
#define ZKERNEL_POLLOUT 2
                uint32_t event_mask = 0;
                if ((rc & ZKERNEL_POLLIN) == ZKERNEL_POLLIN)
                    event_mask |= EPOLLIN | EPOLLONESHOT | EPOLLET;
                if ((rc & ZKERNEL_POLLOUT) == ZKERNEL_POLLOUT)
                    event_mask |= EPOLLOUT | EPOLLONESHOT | EPOLLET;
                if (ev_src->event_mask != event_mask) {
                    struct epoll_event ev = {
                        .events = event_mask,
                        .data = ev_src
                    };
                    const int rc = epoll_ctl (
                        self->poll_fd, EPOLL_CTL_MOD, ev_src->fd, &ev);
                    assert (rc == 0);
                    ev_src->event_mask = event_mask;
                }
            }
        }
    }

    return NULL;
}

//  We should generate response
//  There should be a way to release all event sources on
//  reactor termination
static int
s_register (reactor_t *self, int fd, io_handler_t *handler)
{
    assert (self);
    if (fd == -1)
        return -1;
    struct event_source *event_source = malloc (sizeof *event_source);
    if (!event_source)
        return -1;
    *event_source = (struct event_source) {
        .fd = fd,
        .event_mask = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLONESHOT,
        .handler = *handler
    };
    //  is epollet ok (what happens if data are already ready)?
    struct epoll_event ev = {
        .events = event_source->event_mask,
        .data = event_source
    };
    const int rc = epoll_ctl (self->poll_fd, EPOLL_CTL_ADD, fd, &ev);
    assert (rc == 0);

    return 0;
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
        prev = atomic_ptr_cas (&self->lifo, tail, msg);
    }
    //  Wake up I/O thread if necessary
    if (!prev) {
        uint64_t v = 1;
        const int rc = write (self->ctrl_fd, &v, sizeof v);
        assert (rc == sizeof v);
    }
    return 0;
}
