//  TCP connector class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#define _POSIX_C_SOURCE 1

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "mailbox.h"
#include "io_object.h"
#include "tcp_connector.h"
#include "zkernel.h"

struct tcp_connector {
    io_object_t base;
    struct addrinfo *addrinfo;
    int fd;
    protocol_engine_constructor_t *protocol_engine_constructor;
    int err;
    mailbox_t *owner;
};

static int
    io_init (io_object_t *self_, int *fd, uint32_t *timer_interval);

static int
    io_event (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval);

static int
    io_timeout (io_object_t *self_, int *fd, uint32_t *timer_interval);

static struct io_object_ops ops = {
    .init  = io_init,
    .event = io_event,
    .timeout = io_timeout
};

tcp_connector_t *
tcp_connector_new (protocol_engine_constructor_t *protocol_engine_constructor, mailbox_t *owner)
{
    tcp_connector_t *self = malloc (sizeof *self);
    if (self)
        *self = (tcp_connector_t) {
            .base.ops = ops,
            .fd = -1,
            .protocol_engine_constructor = protocol_engine_constructor,
            .owner = owner
        };
    return self;
}

void
tcp_connector_destroy (tcp_connector_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        tcp_connector_t *self = *self_p;
        if (self->fd != -1 && self->err != 0) {
            const int rc = close (self->fd);
            assert (rc == 0);
        }
        freeaddrinfo (self->addrinfo);
        free (self);
        *self_p = NULL;
    }
}

int
tcp_connector_connect (tcp_connector_t *self, unsigned short port)
{
    assert (self);

    if (self->fd != -1)
        return -1;

    //  Create socket
    const int s = socket (AF_INET, SOCK_STREAM, 0);
    if (s == -1)
        return -1;
    //  Resolve address
    const struct addrinfo hints = {
        .ai_family   = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_flags    = AI_NUMERICHOST | AI_NUMERICSERV
    };
    char service [8 + 1];
    snprintf (service, sizeof service, "%u", port);
    struct addrinfo *addrinfo = NULL;
    if (getaddrinfo ("127.0.0.1", service, &hints, &addrinfo)) {
        close (s);
        return -1;
    }
    assert (addrinfo);
    //  Set non-blocking mode
    const int flags = fcntl (s, F_GETFL, 0);
    assert (flags != -1);
    int rc = fcntl (s, F_SETFL, flags | O_NONBLOCK);
    assert (rc == 0);
    //  Initiate TCP connection
    rc = connect (s, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if (rc == 0) {
        freeaddrinfo (addrinfo);
        return s;
    }
    else
    if (errno == EINPROGRESS) {
        self->addrinfo = addrinfo;
        self->fd = s;
        self->err = errno;
        return -1;
    }
    else {
        const int rc = close (s);
        assert (rc);
        self->addrinfo = addrinfo;
        self->fd = -1;
        self->err = errno;
        return -1;
    }
}

int
tcp_connector_errno (tcp_connector_t *self)
{
    assert (self);
    return self->err;
}

static int
io_init (io_object_t *self_, int *fd, uint32_t *timer_interval)
{
    tcp_connector_t *self = (tcp_connector_t *) self_;
    assert (self);

    *fd = self->fd;
    return 3;
}

static int
io_event (io_object_t *self_, uint32_t flags, int *fd, uint32_t *timer_interval)
{
    tcp_connector_t *self = (tcp_connector_t *) self_;
    assert (self);

    socklen_t len = sizeof self->err;
    const int rc = getsockopt (
       self->fd, SOL_SOCKET, SO_ERROR, &self->err, &len);
    assert (rc == 0);
    if (self->err == 0) {
        *fd = -1;
        return 0;
    }
    else
    if (self->err == EINPROGRESS)
        return 3;
    else {
        close (self->fd);
        *fd = self->fd = -1;
        *timer_interval = 2500;
        return 0;
    }
}

static int
io_timeout (io_object_t *self_, int *fd, uint32_t *timer_interval)
{
    tcp_connector_t *self = (tcp_connector_t *) self_;
    assert (self);

    //  Create socket
    const int s = socket (AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        *timer_interval = 1000;
        return 0;
    }
    //  Set non-blocking mode
    const int flags = fcntl (s, F_GETFL, 0);
    assert (flags != -1);
    int rc = fcntl (s, F_SETFL, flags | O_NONBLOCK);
    assert (rc == 0);
    //  Initiate TCP connection
    rc = connect (s, self->addrinfo->ai_addr, self->addrinfo->ai_addrlen);
    if (rc == 0) {
        return 0;
    }
    else
    if (errno == EINPROGRESS) {
        *fd = self->fd = s;
        return 3;
    }
    else {
        const int rc = close (s);
        assert (rc);
        self->err == errno;
        *timer_interval = 2500;
        return 0;
    }
}
