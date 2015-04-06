/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "pdu.h"
#include "iobuf.h"
#include "zkernel.h"
#include "protocol_engine.h"
#include "zmtp_v1_exchange_id.h"

struct zmtp_v1_exchange_id {
    protocol_engine_t base;
    iobuf_t *sendbuf;
    iobuf_t *recvbuf;
    protocol_engine_t *next_stage;
};

typedef struct zmtp_v1_exchange_id zmtp_v1_exchange_id_t;

static struct protocol_engine_ops ops;

zmtp_v1_exchange_id_t *
s_new (const char *id, size_t peer_id_length)
{
    zmtp_v1_exchange_id_t *self =
        (zmtp_v1_exchange_id_t *) malloc (sizeof *self);
    if (self) {
        const size_t id_length = id ? strlen (id) : 0;
        *self = (zmtp_v1_exchange_id_t) {
            .base.ops = ops,
            .sendbuf = iobuf_new (id_length),
            .recvbuf = iobuf_new (peer_id_length),
        };

        if (self->sendbuf == NULL || self->recvbuf == NULL) {
            iobuf_destroy (&self->sendbuf);
            iobuf_destroy (&self->recvbuf);
            free (self);
            self = NULL;
        }
        else {
            const size_t n = iobuf_write (self->sendbuf, id, id_length);
            assert (n == id_length);
        }
    }

    return self;
}

static int
s_init (protocol_engine_t *base, protocol_engine_info_t *info)
{
    assert (base);

    *info = (protocol_engine_info_t) {
        .flags = ZKERNEL_READ_OK | ZKERNEL_WRITE_OK,
    };

    return 0;
}

static int
s_read (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    zmtp_v1_exchange_id_t *self = (zmtp_v1_exchange_id_t *) base;
    assert (self);

    iobuf_copy_all (iobuf, self->sendbuf);
    unsigned int flags = 0;
    if (iobuf_available (self->sendbuf) > 0)
        flags |= ZKERNEL_READ_OK;
    if (iobuf_space (self->recvbuf) > 0)
        flags |= ZKERNEL_WRITE_OK;
    if (flags == 0)
        flags |= ZKERNEL_ENGINE_DONE;

    *info = (protocol_engine_info_t) { .flags = flags };

    return 0;
}

static int
s_write (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    zmtp_v1_exchange_id_t *self = (zmtp_v1_exchange_id_t *) base;
    assert (self);

    iobuf_copy_all (self->recvbuf, iobuf);
    unsigned int flags = 0;
    if (iobuf_available (self->sendbuf) > 0)
        flags |= ZKERNEL_READ_OK;
    if (iobuf_space (self->recvbuf) > 0)
        flags |= ZKERNEL_WRITE_OK;
    if (flags == 0)
        flags |= ZKERNEL_ENGINE_DONE;

    *info = (protocol_engine_info_t) { .flags = flags };

    return 0;
}

static int
s_next (protocol_engine_t **base_p, protocol_engine_info_t *info)
{
    assert (base_p);
    if (*base_p) {
        zmtp_v1_exchange_id_t *self = (zmtp_v1_exchange_id_t *) *base_p;
        iobuf_destroy (&self->sendbuf);
        iobuf_destroy (&self->recvbuf);
        *base_p = self->next_stage;
        free (self);
    }
}

static void
s_destroy (protocol_engine_t **base_p)
{
    assert (base_p);
    if (*base_p) {
        zmtp_v1_exchange_id_t *self = (zmtp_v1_exchange_id_t *) *base_p;
        iobuf_destroy (&self->sendbuf);
        iobuf_destroy (&self->recvbuf);
        *base_p = NULL;
        free (self);
    }
}

static struct protocol_engine_ops ops = {
    .init = s_init,
    .read = s_read,
    .write = s_write,
    .next = s_next,
    .destroy = s_destroy,
};

protocol_engine_t *
zmtp_v1_exchange_id_new_protocol_engine (const char *id, size_t peer_id_length)
{
    return (protocol_engine_t *) s_new (id, peer_id_length);
}

