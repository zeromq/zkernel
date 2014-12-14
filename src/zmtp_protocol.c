//  ZMTP handshaker class

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "zmtp_protocol.h"

static const int zmtp_1_0   = 0;
static const int zmtp_2_0   = 1;
static const int zmtp_3_0   = 3;

static const size_t zmtp_signature_size     = 10;
static const size_t zmtp_version_offset     = 10;
static const size_t zmtp_v2_greeting_size   = 12;
static const size_t zmtp_v3_greeting_size   = 64;

struct state {
    struct state (*fn) (zmtp_protocol_t *, iobuf_t *, iobuf_t *);
};

typedef struct state state_t;

typedef state_t (state_fn_t) (zmtp_protocol_t *, iobuf_t *, iobuf_t *);

struct zmtp_protocol {
    protocol_t base;
    int errno;
    bool handshake_complete;
    state_t state;
};

static int
s_handshake (protocol_t *base, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    zmtp_protocol_t *self = (zmtp_protocol_t *) base;
    assert (self);

    state_t state = self->state.fn (self, recvbuf, sendbuf);
    while (state.fn != self->state.fn) {
        self->state = state;
        state = self->state.fn (self, recvbuf, sendbuf);
    }

    return self->errno;
}

static bool
s_is_handshake_complete (protocol_t *base)
{
    zmtp_protocol_t *self = (zmtp_protocol_t *) base;
    assert (self);

    return self->handshake_complete;
}

static encoder_t *
s_encoder (protocol_t *base, encoder_info_t *encoder_info)
{
    return NULL;
}

static decoder_t *
s_decoder (protocol_t *base, decoder_info_t *decoder_info)
{
    return NULL;
}

static size_t
s_min_buffer_size (protocol_t *self)
{
    return zmtp_v3_greeting_size;
}

static void
s_destroy (protocol_t **self_p)
{
    if (self_p) {
        zmtp_protocol_t *self = (zmtp_protocol_t *) *self_p;
        assert (self);
        free (self);
        *self_p = NULL;
    }
}

static state_fn_t send_signature;

zmtp_protocol_t *
zmtp_protocol_new ()
{
    zmtp_protocol_t *self = malloc (sizeof *self);
    if (self) {
        *self = (zmtp_protocol_t) {
            .base = {
                .handshake = s_handshake,
                .is_handshake_complete = s_is_handshake_complete,
                .encoder = s_encoder,
                .decoder = s_decoder,
                .min_buffer_size = s_min_buffer_size,
                .destroy = s_destroy
            },
            .state.fn = send_signature
        };
    }
    return self;
}

static state_fn_t
    receive_signature,
    receive_zmtp_version,
    receive_zmtp_v2_greeting,
    receive_zmtp_v3_greeting;

static state_t
send_signature (
    zmtp_protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    const uint8_t signature [] = { 255, 0, 0, 0, 0, 0, 0, 0, 1, 127 };
    const size_t n = iobuf_write (sendbuf, signature, zmtp_signature_size);
    assert (n == zmtp_signature_size);

    return (state_t) { receive_signature };
}

static state_t
receive_signature (
    zmtp_protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    if (iobuf_available (recvbuf) > 0) {
        if (recvbuf->base [0] != 0xff)
            //  send identity
            return (state_t) { receive_signature };
    }
    if (iobuf_available (recvbuf) >= zmtp_signature_size) {
        if ((recvbuf->base [9] & 0x01) == 0)
            //  send identity
            return (state_t) { receive_signature };
        else {
            const size_t n = iobuf_write_byte (sendbuf, zmtp_3_0);
            assert (n == 1);
            return (state_t) { receive_zmtp_version };
        }
    }

    return (state_t) { receive_signature };
}

static state_t
receive_zmtp_version (
    zmtp_protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    if (iobuf_available (recvbuf) > zmtp_signature_size) {
        //  Use ZMTP/2.0.
        if (recvbuf->base [zmtp_version_offset] == zmtp_1_0
        ||  recvbuf->base [zmtp_version_offset] == zmtp_2_0) {
            const int rc = iobuf_write_byte (sendbuf, 0);
            assert (rc == 1);
            return (state_t) { receive_zmtp_v2_greeting };
        }
        else {
            //  Minor version number
            size_t n = iobuf_write_byte (sendbuf, 0);
            assert (n == 0);

            char mechanism [20] = { 0 };
            memcpy (mechanism + 16, "NULL", 4);
            n = iobuf_write (sendbuf, mechanism, sizeof mechanism);
            assert (n == sizeof mechanism);

            n = iobuf_write_byte (sendbuf, 0);
            assert (n == 1);

            const char filler [31] = { 0 };
            n = iobuf_write (sendbuf, filler, sizeof filler);
            assert (n == sizeof filler);

            return (state_t) { receive_zmtp_v3_greeting };
        }
    }

    return (state_t) { receive_zmtp_version };
}

static state_t
receive_zmtp_v2_greeting (
    zmtp_protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    if (iobuf_available (recvbuf) >= zmtp_v2_greeting_size)
        self->handshake_complete = true;

    return (state_t) { receive_zmtp_v2_greeting };
}

static state_t
receive_zmtp_v3_greeting (
    zmtp_protocol_t *self, iobuf_t *recvbuf, iobuf_t *sendbuf)
{
    if (iobuf_available (recvbuf) >= zmtp_v3_greeting_size)
        self->handshake_complete = true;

    return (state_t) { receive_zmtp_v3_greeting };
}
