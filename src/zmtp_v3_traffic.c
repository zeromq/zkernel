//  ZMTP v3 protocol class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <stdlib.h>
#include <assert.h>

#include "zmtp_v2_frame_encoder.h"
#include "zmtp_v3_decoder.h"
#include "protocol_engine.h"

struct zmtp_v3_protocol_engine {
    protocol_engine_t base;
    zmtp_v2_frame_encoder_t *encoder;
    zmtp_v2_frame_encoder_info_t encoder_info;
    zmtp_v3_decoder_t *decoder;
    zmtp_v3_decoder_info_t decoder_info;
};

typedef struct zmtp_v3_protocol_engine zmtp_v3_protocol_engine_t;

static struct protocol_engine_ops ops;

static zmtp_v3_protocol_engine_t *
s_new ()
{
    zmtp_v3_protocol_engine_t *self =
        (zmtp_v3_protocol_engine_t *) malloc (sizeof *self);
    if (self) {
        *self = (zmtp_v3_protocol_engine_t) {
            .base.ops = ops,
        };

        self->encoder = zmtp_v2_frame_encoder_new (&self->encoder_info);
        self->decoder = zmtp_v3_decoder_new (&self->decoder_info);

        if (self->encoder == NULL || self->decoder == NULL) {
            zmtp_v2_frame_encoder_destroy (&self->encoder);
            zmtp_v3_decoder_destroy (&self->decoder);
            free (self);
            self = NULL;
        }
    }

    return self;
}

static int
s_init (protocol_engine_t *base, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static int
s_encode (protocol_engine_t *base, pdu_t *pdu, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    const int rc =
        zmtp_v2_frame_encoder_putmsg (self->encoder, pdu, encoder_info);
    if (rc == -1)
        return -1;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static pdu_t *
s_decode (protocol_engine_t *base, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    pdu_t *pdu =
        zmtp_v3_decoder_getmsg (self->decoder, decoder_info);
    if (pdu == NULL)
        return pdu;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static int
s_read (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    const int rc =
        zmtp_v2_frame_encoder_read (self->encoder, iobuf, encoder_info);
    if (rc == -1)
        return -1;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static int
s_read_advance (protocol_engine_t *base, size_t n, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    const int rc =
        zmtp_v2_frame_encoder_advance (self->encoder, n, encoder_info);
    if (rc == -1)
        return -1;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static int
s_write (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    const int rc =
        zmtp_v3_decoder_write (self->decoder, iobuf, decoder_info);
    if (rc == -1)
        return -1;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static int
s_write_advance (protocol_engine_t *base, size_t n, protocol_engine_info_t *info)
{
    zmtp_v3_protocol_engine_t *self = (zmtp_v3_protocol_engine_t *) base;
    assert (self);

    zmtp_v2_frame_encoder_info_t *encoder_info =
        &self->encoder_info;
    zmtp_v3_decoder_info_t *decoder_info =
        &self->decoder_info;

    const int rc =
        zmtp_v3_decoder_advance (self->decoder, n, decoder_info);
    if (rc == -1)
        return -1;

    *info = (protocol_engine_info_t) {
        .flags = encoder_info->flags | decoder_info->flags,
        .read_buffer = encoder_info->buffer,
        .read_buffer_size = encoder_info->buffer_size,
        .write_buffer = decoder_info->buffer,
        .write_buffer_size = decoder_info->buffer_size,
    };

    return 0;
}

static void
s_destroy (protocol_engine_t **base_p)
{
    assert (base_p);
    if (*base_p) {
        zmtp_v3_protocol_engine_t *self =
            (zmtp_v3_protocol_engine_t *) *base_p;
        zmtp_v2_frame_encoder_destroy (&self->encoder);
        zmtp_v3_decoder_destroy (&self->decoder);
        free (self);
        *base_p = NULL;
    }
}

static struct protocol_engine_ops ops = {
    .init = s_init,
    .encode = s_encode,
    .read = s_read,
    .read_advance = s_read_advance,
    .decode = s_decode,
    .write = s_write,
    .write_advance = s_write_advance,
    .destroy = s_destroy,
};

protocol_engine_t *
zmtp_v3_traffic_protocol_engine ()
{
    return (protocol_engine_t *) s_new ();
}
