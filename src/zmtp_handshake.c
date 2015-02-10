//  ZMTP handshake class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "zmtp_handshake.h"

static const int zmtp_1_0   = 0;
static const int zmtp_2_0   = 1;
static const int zmtp_3_0   = 3;

static const size_t zmtp_signature_size     = 10;
static const size_t zmtp_version_offset     = 10;
static const size_t zmtp_v2_greeting_size   = 12;
static const size_t zmtp_v3_greeting_size   = 64;

struct state {
    struct state (*write_fn) (zmtp_handshake_t *, iobuf_t *);
};

typedef struct state state_t;

struct zmtp_handshake {
    protocol_engine_t base;
    int error;
    state_t state;
    iobuf_t *sendbuf;
    iobuf_t *recvbuf;
    protocol_engine_t *next_stage;
};

typedef state_t (state_fn_t) (zmtp_handshake_t *, iobuf_t *);

static state_fn_t
    receive_signature_a,
    receive_signature_b,
    receive_zmtp_version,
    receive_zmtp_v2_greeting,
    receive_zmtp_v3_greeting;

static struct protocol_engine_ops protocol_engine_ops;

zmtp_handshake_t *
zmtp_handshake_new ()
{
    zmtp_handshake_t *self =
        (zmtp_handshake_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_handshake_t) {
            .base.ops = protocol_engine_ops,
            .state.write_fn = receive_signature_a,
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
s_init (protocol_engine_t *base, uint32_t *status)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    const uint8_t signature [] = { 0xff, 0, 0, 0, 0, 0, 0, 0, 1, 0x7f };
    iobuf_write (self->sendbuf, signature, sizeof signature);
    assert (iobuf_available (self->sendbuf) == sizeof signature);

    *status = ZKERNEL_PROTOCOL_ENGINE_READ_OK | ZKERNEL_PROTOCOL_ENGINE_WRITE_OK;

    return 0;
}

static int
s_encode (protocol_engine_t *base, pdu_t *pdu, uint32_t *status)
{
    return -1;
}

static int
s_read (protocol_engine_t *base, iobuf_t *iobuf, uint32_t *status)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    iobuf_copy_all (iobuf, self->sendbuf);
    *status = 0;
    if (iobuf_available (self->sendbuf) > 0)
        *status |= ZKERNEL_PROTOCOL_ENGINE_READ_OK;
    if (self->state.write_fn != NULL)
        *status |= ZKERNEL_PROTOCOL_ENGINE_WRITE_OK;
    /*
    if (self->next_stage != NULL)
        *status |= ZKERNEL_PROTOCOL_ENGINE_NEXT_STAGE;
    */

    return 0;
}

static int
s_read_buffer (protocol_engine_t *base, const void **buffer, size_t *buffer_size)
{
    return -1;
}

static int
s_read_advance (protocol_engine_t *base, size_t n, uint32_t *status)
{
    return -1;
}

static pdu_t *
s_decode (protocol_engine_t *base, uint32_t *status)
{
    return NULL;
}

static int
s_write (protocol_engine_t *base, iobuf_t *iobuf, uint32_t *status)
{
    zmtp_handshake_t *self = (zmtp_handshake_t *) base;
    assert (self);

    if (self->state.write_fn == NULL)
        return -1;

    self->state = self->state.write_fn (self, iobuf);
    if (self->state.write_fn == NULL) {
        if (self->error)
            return -1;
        assert (self->next_stage);
        if (iobuf_available (self->sendbuf) == 0)
            *status = 0;  //  TODO: Indicate next stage is ready
        else
            *status |= ZKERNEL_PROTOCOL_ENGINE_READ_OK;
    }
    else {
        *status = ZKERNEL_PROTOCOL_ENGINE_WRITE_OK;
        if (iobuf_available (self->sendbuf) > 0)
            *status |= ZKERNEL_PROTOCOL_ENGINE_READ_OK;
    }

    return 0;
}

static int
s_write_buffer (protocol_engine_t *base, void **buffer, size_t *buffer_size)
{
    return -1;
}

static int
s_write_advance (protocol_engine_t *base, size_t n, uint32_t *status)
{
    return -1;
}

static void
s_destroy (protocol_engine_t **base_p)
{
    assert (base_p);
    if (*base_p) {
        zmtp_handshake_t *self = (zmtp_handshake_t *) *base_p;
        iobuf_destroy (&self->sendbuf);
        iobuf_destroy (&self->recvbuf);
        free (self);
        *base_p = NULL;
    }
}

static state_t
receive_signature_a (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    iobuf_copy (self->recvbuf, iobuf, 1);
    if (iobuf_available (self->recvbuf) < 1)
        return (state_t) { receive_signature_a };

    if (self->recvbuf->base [0] != 0xff) {
        //  send identity
        return (state_t) { NULL };
    }

    return receive_signature_b (self, iobuf);
}

static state_t
receive_signature_b (zmtp_handshake_t *self, iobuf_t *iobuf)
{
    size_t n = zmtp_signature_size - iobuf_available (self->recvbuf);
    iobuf_copy (self->recvbuf, iobuf, n);
    if (iobuf_available (self->recvbuf) < zmtp_signature_size)
        return (state_t) { receive_signature_b };

    if ((self->recvbuf->base [9] & 0x01) == 0) {
        //  send identity
        return (state_t) { NULL };
    }

    n = iobuf_write_byte (self->sendbuf, zmtp_3_0);
    assert (n == 1);

    return receive_zmtp_version (self, iobuf);
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

    return (state_t) { NULL };
}

static struct protocol_engine_ops protocol_engine_ops = {
    .init = s_init,
    .encode = s_encode,
    .read = s_read,
    .read_buffer = s_read_buffer,
    .read_advance = s_read_advance,
    .decode = s_decode,
    .write = s_write,
    .write_buffer = s_write_buffer,
    .write_advance = s_write_advance,
    .destroy = s_destroy,
};
