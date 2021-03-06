//  ZMTP handshake class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "zkernel.h"
#include "zmtp_utils.h"
#include "zmtp_handshake.h"
#include "zmtp_v1_exchange_id.h"
#include "zmtp_null_handshake.h"

static const int zmtp_1_0   = 0;
static const int zmtp_2_0   = 1;
static const int zmtp_3_0   = 3;

static const size_t zmtp_signature_size     = 10;
static const size_t zmtp_version_offset     = 10;
static const size_t zmtp_v2_greeting_size   = 12;
static const size_t zmtp_v3_greeting_size   = 64;

struct state {
    struct state (*write) (zmtp_handshake_t *, iobuf_t *);
};

typedef struct state state_t;

struct zmtp_handshake {
    protocol_engine_t base;
    state_t state;
    iobuf_t *sendbuf;
    iobuf_t *recvbuf;
    char *socket_id;
    protocol_engine_t *next_stage;
};

typedef state_t (state_fn_t) (zmtp_handshake_t *, iobuf_t *);

static state_fn_t
    receive_signature_a,
    receive_signature_b,
    receive_zmtp_version,
    receive_zmtp_v2_greeting,
    receive_zmtp_v3_greeting;

static struct protocol_engine_ops ops;

zmtp_handshake_t *
zmtp_handshake_new ()
{
    zmtp_handshake_t *self =
        (zmtp_handshake_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_handshake_t) {
            .base.ops = ops,
            .state.write = receive_signature_a,
            .sendbuf = iobuf_new (zmtp_v3_greeting_size),
            .recvbuf = iobuf_new (zmtp_v3_greeting_size),
        };

        if (self->sendbuf == NULL || self->recvbuf == NULL) {
            iobuf_destroy (&self->sendbuf);
            iobuf_destroy (&self->recvbuf);
            free (self);
            self = NULL;
        }
    }

    return self;
}

protocol_engine_t *
zmtp_handshake_new_protocol_engine ()
{
    return (protocol_engine_t *) zmtp_handshake_new ();
}

static int
s_init (protocol_engine_t *base, protocol_engine_info_t *info)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    const size_t socket_id_len =
        self->socket_id ? strlen (self->socket_id) : 0;

    uint8_t signature [] = { 0xff, 0, 0, 0, 0, 0, 0, 0, 1, 0x7f };
    put_uint64 (signature + 1, socket_id_len + 1);
    iobuf_write (self->sendbuf, signature, sizeof signature);
    assert (iobuf_available (self->sendbuf) == sizeof signature);

    *info = (protocol_engine_info_t) {
        .flags = ZKERNEL_READ_OK | ZKERNEL_WRITE_OK,
    };

    return 0;
}

static int
s_read (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    iobuf_copy_all (iobuf, self->sendbuf);
    unsigned int flags = 0;
    if (iobuf_available (self->sendbuf) > 0) {
        flags |= ZKERNEL_READ_OK;
        if (self->state.write != NULL)
            flags |= ZKERNEL_WRITE_OK;
    }
    else
    if (self->state.write != NULL)
        flags |= ZKERNEL_WRITE_OK;
    else
        flags |= ZKERNEL_ENGINE_DONE;
    *info = (protocol_engine_info_t) { .flags = flags };

    return 0;
}

static int
s_write (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    if (self->state.write == NULL)
        return -1;

    self->state = self->state.write (self, iobuf);
    unsigned int flags = 0;
    if (iobuf_available (self->sendbuf) > 0) {
        flags |= ZKERNEL_READ_OK;
        if (self->state.write != NULL)
            flags |= ZKERNEL_WRITE_OK;
    }
    else
    if (self->state.write != NULL)
        flags |= ZKERNEL_WRITE_OK;
    else
        flags |= ZKERNEL_ENGINE_DONE;
    *info = (protocol_engine_info_t) { .flags = flags };

    return 0;
}

static int
s_set_socket_id (protocol_engine_t *base, const char *socket_id)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    if (socket_id == NULL || socket_id [0] == '\0') {
        free (self->socket_id);
        self->socket_id = NULL;
    }
    else {
        char *s = malloc (strlen (socket_id) + 1);
        if (s == NULL)
            return -1;
        strcpy (s, socket_id);
        free (self->socket_id);
        self->socket_id = s;
    }

    return 0;
}

static int
s_next (protocol_engine_t **base_p, protocol_engine_info_t *info)
{
    assert (base_p);
    if (*base_p) {
        zmtp_handshake_t *self = (zmtp_handshake_t *) *base_p;
        iobuf_destroy (&self->sendbuf);
        iobuf_destroy (&self->recvbuf);
        free (self->socket_id);
        *base_p = self->next_stage;
        free (self);
    }
}

static void
s_destroy (protocol_engine_t **base_p)
{
    assert (base_p);
    if (*base_p) {
        zmtp_handshake_t *self = (zmtp_handshake_t *) *base_p;
        iobuf_destroy (&self->sendbuf);
        iobuf_destroy (&self->recvbuf);
        free (self->socket_id);
        *base_p = NULL;
        free (self);
    }
}

static state_t
receive_signature_a (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    iobuf_copy (self->recvbuf, iobuf, 1);
    if (iobuf_available (self->recvbuf) < 1)
        return (state_t) { receive_signature_a };
    else
    if (self->recvbuf->base [0] == 0xff)
        return receive_signature_b (self, iobuf);
    else {
        const uint64_t peer_id_length =
            get_uint64 (self->recvbuf->base + 1);
        if (peer_id_length > 0)
            self->next_stage = zmtp_v1_exchange_id_new_protocol_engine (
                     self->socket_id, peer_id_length - 1);
        return (state_t) { NULL };
    }
}

static state_t
receive_signature_b (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    const size_t n = zmtp_signature_size - iobuf_available (self->recvbuf);
    iobuf_copy (self->recvbuf, iobuf, n);
    if (iobuf_available (self->recvbuf) < zmtp_signature_size)
        return (state_t) { receive_signature_b };
    else
    if ((self->recvbuf->base [9] & 0x01) == 0x01) {
        const size_t n = iobuf_write_byte (self->sendbuf, zmtp_3_0);
        assert (n == 1);
        return receive_zmtp_version (self, iobuf);
    }
    else {
        const uint64_t peer_id_length =
            get_uint64 (self->recvbuf->base + 1);
        if (peer_id_length > 0)
            self->next_stage = zmtp_v1_exchange_id_new_protocol_engine (
                     self->socket_id, peer_id_length - 1);
        return (state_t) { NULL };
    }
}

static state_t
receive_zmtp_version (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    const size_t n = iobuf_copy (self->recvbuf, iobuf, 1);
    if (n == 0)
        return (state_t) { receive_zmtp_version };

    const uint8_t zmtp_version =
        self->recvbuf->base [zmtp_version_offset];

    if (zmtp_version == zmtp_1_0 || zmtp_version == zmtp_2_0) {
        const size_t rc = iobuf_write_byte (self->sendbuf, 0);
        assert (rc == 1);
        return receive_zmtp_v2_greeting (self, iobuf);
    }
    else {
        size_t n = iobuf_write_byte (self->sendbuf, 0);
        assert (n == 1);
        char mechanism [20] = { 'N', 'U', 'L', 'L' };
        n = iobuf_write (self->sendbuf, mechanism, sizeof mechanism);
        assert (n == sizeof mechanism);
        n = iobuf_write_byte (self->sendbuf, 0);
        assert (n == 1);
        const char filler [31] = { 0 };
        n = iobuf_write (self->sendbuf, filler, sizeof filler);
        assert (n == sizeof filler);
        return receive_zmtp_v3_greeting (self, iobuf);
    }
}

static state_t
receive_zmtp_v2_greeting (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    const size_t n = zmtp_v2_greeting_size - iobuf_available (self->recvbuf);
    iobuf_copy (self->recvbuf, iobuf, n);

    if (iobuf_available (self->recvbuf) < zmtp_v2_greeting_size)
        return (state_t) { receive_zmtp_v2_greeting };

    return (state_t) { NULL };
}

static state_t
receive_zmtp_v3_greeting (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    const size_t n = zmtp_v3_greeting_size - iobuf_available (self->recvbuf);
    iobuf_copy (self->recvbuf, iobuf, n);

    if (iobuf_available (self->recvbuf) < zmtp_v3_greeting_size)
        return (state_t) { receive_zmtp_v3_greeting };

    self->next_stage = zmtp_null_handshake_new_protocol_engine ();
    return (state_t) { NULL };
}

static struct protocol_engine_ops ops = {
    .init = s_init,
    .read = s_read,
    .write = s_write,
    .set_socket_id = s_set_socket_id,
    .next = s_next,
    .destroy = s_destroy,
};
