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
#include "clock.h"

struct event_source {
    int fd;
    uint64_t timer;
    uint32_t event_mask;
    io_handler_t handler;
};

struct timer {
    uint64_t t;
    struct event_source *ev_src;
};

struct reactor {
    int poll_fd;
    int ctrl_fd;
    struct event_source controler;
    void *lifo;
    pthread_t thread_handle;
    struct timer timers [8];
};

static void *
    s_loop (void *udata);

static int
    s_send_msg (void *self_, msg_t *msg);

static int
    s_register (reactor_t *self, io_handler_t *handler);


static struct timer *
    s_alloc_timer (reactor_t *self);

static void
    s_free_timer (struct timer *timer);

struct timer *
    s_next_timer (reactor_t *self);

struct timer *
    s_find_timer (reactor_t *self, struct event_source *ev_src);

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
        .controler = { .fd = ctrl_fd, .event_mask = EPOLLIN }
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
        .ftab = { .enqueue = s_send_msg }
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

    uint64_t now = clock_now ();
    while (!stop) {
        struct timer *next_timer = s_next_timer (self);
        const int max_wait =
            next_timer == NULL
                ? -1
                : (next_timer->t < now? 0: next_timer->t - now);
        const int nfds = epoll_wait (
            self->poll_fd, events, MAX_EVENTS, max_wait);
        now = clock_now ();
        if (nfds == -1) {
            assert (errno == EINTR);
            continue;
        }
        bool msg_flag = false;
        for (int i = 0; i < nfds; i++) {
            const uint32_t what = events [i].events;
            struct event_source *ev_src =
                (struct event_source *) events [i].data.ptr;
            if (ev_src->fd == self->ctrl_fd)
                msg_flag = true;
            else {
                uint32_t flags = 0;
#define ZKERNEL_INPUT_READY     0x01
#define ZKERNEL_OUTPUT_READY    0x02
#define ZKERNEL_IO_ERROR        0x04
                if ((what & EPOLLIN) == EPOLLIN) {
                    flags |= ZKERNEL_INPUT_READY;
                    ev_src->event_mask = 0;
                }
                if ((what & EPOLLOUT) == EPOLLOUT) {
                    flags |= ZKERNEL_OUTPUT_READY;
                    ev_src->event_mask = 0;
                }
                if ((what & (EPOLLERR | EPOLLHUP)) != 0) {
                    flags |= ZKERNEL_IO_ERROR;
                    ev_src->event_mask = 0;
                }
                int fd = ev_src->fd;
                uint32_t timer_interval = 0;
                const int rc = io_handler_event (
                    &ev_src->handler, flags, &fd, &timer_interval);
                if (timer_interval > 0) {
                    struct timer *timer = NULL;
                    if (ev_src->timer != 0)
                        timer = s_find_timer (self, ev_src);
                    else
                        timer = s_alloc_timer (self);
                    timer->t = now + timer_interval;
                    timer->ev_src = ev_src;
                    ev_src->timer = timer->t;
                }
#define ZKERNEL_POLLIN 1
#define ZKERNEL_POLLOUT 2
                uint32_t event_mask = 0;
                if ((rc & ZKERNEL_POLLIN) == ZKERNEL_POLLIN)
                    event_mask |= EPOLLIN | EPOLLONESHOT;
                if ((rc & ZKERNEL_POLLOUT) == ZKERNEL_POLLOUT)
                    event_mask |= EPOLLOUT | EPOLLONESHOT;
                if (fd != ev_src->fd) {
                    struct epoll_event ev = {};
                    const int rc = epoll_ctl (
                        self->poll_fd, EPOLL_CTL_DEL, ev_src->fd, &ev);
                    assert (rc == 0 || errno == ENOENT);
                    if (fd != -1) {
                        struct epoll_event ev = {
                            .events = event_mask,
                            .data = ev_src
                        };
                        const int rc = epoll_ctl (
                            self->poll_fd, EPOLL_CTL_ADD, fd, &ev);
                        assert (rc == 0);
                    }
                    ev_src->event_mask = event_mask;
                    ev_src->fd = fd;
                }
                else
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
        struct timer *timer = s_next_timer (self);
        while (timer && timer->t <= now) {
            struct event_source *ev_src = timer->ev_src;
            assert (ev_src);
            int fd = ev_src->fd;
            uint32_t timer_interval = 0;
            const int rc = io_handler_timeout (
                &ev_src->handler, &fd, &timer_interval);
            uint32_t event_mask = 0;
            if ((rc & ZKERNEL_POLLIN) == ZKERNEL_POLLIN)
                event_mask |= EPOLLIN | EPOLLONESHOT;
            if ((rc & ZKERNEL_POLLOUT) == ZKERNEL_POLLOUT)
                event_mask |= EPOLLOUT | EPOLLONESHOT;
            if (fd != ev_src->fd) {
                if (ev_src->fd != -1) {
                    struct epoll_event ev = {};
                    const int rc = epoll_ctl (
                        self->poll_fd, EPOLL_CTL_DEL, ev_src->fd, &ev);
                    assert (rc == 0 || errno == EBADF || errno == ENOENT);
                }
                if (fd != -1) {
                    struct epoll_event ev = {
                        .events = event_mask,
                        .data = ev_src
                    };
                    const int rc = epoll_ctl (
                        self->poll_fd, EPOLL_CTL_ADD, fd, &ev);
                    assert (rc == 0);
                }
                ev_src->event_mask = event_mask;
                ev_src->fd = fd;
            }
            else
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
            if (timer_interval > 0) {
                timer->t = now + timer_interval;
                timer->ev_src = ev_src;
                ev_src->timer = timer->t;
            }
            else
                s_free_timer (timer);
            timer = s_next_timer (self);
        }
        if (msg_flag) {
            struct msg_t *msg =
                (struct msg_t *) atomic_ptr_swap (&self->lifo, NULL);
            assert (msg);
            uint64_t x;
            const int rc = read (self->ctrl_fd, &x, sizeof x);
            assert (rc == sizeof x);
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
                if (msg->cmd == ZKERNEL_KILL) {
                    msg_destroy (&msg);
                    stop = 1;
                }
                else
                if (msg->cmd == ZKERNEL_REGISTER) {
                    s_register (self, &msg->handler);
                    //  Send reply; we send the request back for now
                    mailbox_enqueue (&msg->reply_to, msg);
                }
                else
                    msg_destroy (&msg);
                msg = next_msg;
            }
        }
    }

    return NULL;
}


static int
s_register (reactor_t *self, io_handler_t *handler)
{
    assert (self);

    struct event_source *ev_src = malloc (sizeof *ev_src);
    if (!ev_src)
        return -1;
    *ev_src = (struct event_source) { .fd = -1, .handler = *handler };

    uint32_t timer_interval = 0;
    int rc = io_handler_init (handler, &ev_src->fd, &timer_interval);
    if (ev_src->fd != -1) {
        if ((rc & ZKERNEL_POLLIN) == ZKERNEL_POLLIN)
            ev_src->event_mask |= EPOLLIN | EPOLLONESHOT;
        if ((rc & ZKERNEL_POLLOUT) == ZKERNEL_POLLOUT)
            ev_src->event_mask |= EPOLLOUT | EPOLLONESHOT;

        struct epoll_event ev = {
            .events = ev_src->event_mask,
            .data = ev_src
        };
        rc = epoll_ctl (self->poll_fd, EPOLL_CTL_ADD, ev_src->fd, &ev);
        assert (rc == 0);
    }
    if (timer_interval > 0) {
        struct timer *timer = s_alloc_timer (self);
        assert (timer);
        timer->t = clock_now () + timer_interval;
        timer->ev_src = ev_src;
        ev_src->timer = timer->t;
    }

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

struct timer *
s_alloc_timer (reactor_t *self)
{
    for (int i = 0; i < 8; i++) {
        if (self->timers [i].t == 0)
            return self->timers + i;
    }
    return NULL;
}

void
s_free_timer (struct timer *timer)
{
    timer->t = 0;
}

struct timer *
s_next_timer (reactor_t *self)
{
    struct timer *timer = self->timers;
    for (int i = 1; i < 8; i++)
        if (timer->t < self->timers [i].t)
            timer = self->timers + i;
    return timer->t > 0? timer: NULL;
}

struct timer *
s_find_timer (reactor_t *self, struct event_source *ev_src)
{
    for (int i = 0; i < 8; i++)
        if (self->timers [i].ev_src == ev_src)
            return self->timers + 1;
    return NULL;
}
