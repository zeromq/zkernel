//  ZMTP NULL mechanism handshake class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "protocol_engine.h"
#include "zmtp_v3_encoder.h"
#include "zmtp_v3_decoder.h"
#include "pdu.h"
#include "zmtp_null_handshake.h"

struct zmtp_null_handshake {
    protocol_engine_t base;
    zmtp_v3_encoder_t *encoder;
    zmtp_v3_decoder_t *decoder;
    bool msg_sent;
    bool msg_received;
};

typedef struct zmtp_null_handshake zmtp_null_handshake_t;

static struct protocol_engine_ops ops;

static int
process_msg (zmtp_null_handshake_t *self, pdu_t *pdu);

static zmtp_null_handshake_t *
zmtp_null_handshake_new ()
{
    zmtp_null_handshake_t *self =
        (zmtp_null_handshake_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_null_handshake_t) {
            .base.ops = ops,
            .encoder = zmtp_v3_encoder_new (),
            .decoder = zmtp_v3_decoder_new (),
        };
        if (self->encoder == NULL || self->decoder == NULL) {
            zmtp_v3_encoder_destroy (&self->encoder);
            zmtp_v3_decoder_destroy (&self->decoder);
            free (self);
            self = NULL;
        }
    }

    return self;
}

static int
s_init (protocol_engine_t *base, uint32_t *status)
{
    zmtp_null_handshake_t *self = (zmtp_null_handshake_t *) base;
    assert (self);

    const uint8_t msg [] = { 0x05, 'R', 'E', 'A', 'D', 'Y' };

    pdu_t *pdu = pdu_new_with_size (sizeof msg);
    if (pdu == NULL)
        return -1;

    memcpy (pdu->pdu_data, msg, sizeof msg);

    zmtp_v3_encoder_status_t encoder_status = 0;
    const int rc =
        zmtp_v3_encoder_putmsg (self->encoder, pdu, &encoder_status);
    if (rc == -1)
        return -1;

    assert ((encoder_status & ZMTP_V3_ENCODER_READ_OK) != 0);

    *status = ZKERNEL_PROTOCOL_ENGINE_READ_OK
            | ZKERNEL_PROTOCOL_ENGINE_WRITE_OK;

    return 0;
}

static int
s_read (protocol_engine_t *base, iobuf_t *iobuf, uint32_t *status)
{
    zmtp_null_handshake_t *self = (zmtp_null_handshake_t *) base;
    assert (self);

    if (self->msg_sent)
        return -1;

    zmtp_v3_encoder_status_t encoder_status;
    const int rc =
        zmtp_v3_encoder_read (self->encoder, iobuf, &encoder_status);
    if (rc == -1)
        return -1;
    if ((encoder_status & ZMTP_V3_ENCODER_READ_OK) == 0)
        self->msg_sent = true;

    *status = 0;
    if (!self->msg_sent)
        *status |= ZKERNEL_PROTOCOL_ENGINE_READ_OK;
    if (!self->msg_received)
        *status |= ZKERNEL_PROTOCOL_ENGINE_WRITE_OK;
    if (self->msg_sent && self->msg_received)
        *status |= ZKERNEL_PROTOCOL_ENGINE_DONE;

    return 0;
}

static int
s_write (protocol_engine_t *base, iobuf_t *iobuf, uint32_t *status)
{
    zmtp_null_handshake_t *self = (zmtp_null_handshake_t *) base;
    assert (self);

    if (self->msg_received)
        return -1;

    zmtp_v3_decoder_status_t decoder_status;
    const int rc =
        zmtp_v3_decoder_write (self->decoder, iobuf, &decoder_status);
    if (rc == -1)
        return -1;

    if ((decoder_status & ZMTP_V3_DECODER_READY) != 0) {
        pdu_t *pdu =
            zmtp_v3_decoder_getmsg (self->decoder, &decoder_status);
        if (rc == -1 || pdu == NULL)
            return -1;
        if (process_msg (self, pdu) == -1)
            return -1;
        pdu_destroy (&pdu);
        self->msg_received = true;
    }

    *status = 0;
    if (!self->msg_sent)
        *status |= ZKERNEL_PROTOCOL_ENGINE_READ_OK;
    if (!self->msg_received)
        *status |= ZKERNEL_PROTOCOL_ENGINE_WRITE_OK;
    if (self->msg_sent && self->msg_received)
        *status |= ZKERNEL_PROTOCOL_ENGINE_DONE;

    return 0;
}

static void
s_destroy (protocol_engine_t **base_p)
{
    assert (base_p);
    if (*base_p) {
        zmtp_null_handshake_t *self = (zmtp_null_handshake_t *) *base_p;
        zmtp_v3_encoder_destroy (&self->encoder);
        zmtp_v3_decoder_destroy (&self->decoder);
        free (self);
        *base_p = NULL;
    }
}

static int
process_msg (zmtp_null_handshake_t *self, pdu_t *pdu)
{
    if (pdu->pdu_size >= 6 && memcmp (pdu->pdu_data, "\0x05READY", 6) == 0)
        return 0;
    else
    if (pdu->pdu_size >= 6 && memcmp (pdu->pdu_data, "\0x05ERROR", 6) == 0)
        return 0;
    else
        return -1;
}

static struct protocol_engine_ops ops = {
    .init = s_init,
    .read = s_read,
    .write = s_write,
    .destroy = s_destroy,
};

protocol_engine_t *
zmtp_null_handshake_protocol_engine ()
{
    return (protocol_engine_t *) zmtp_null_handshake_new ();
}
