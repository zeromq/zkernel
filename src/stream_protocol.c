//  Stream protocol engine class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "zkernel.h"
#include "iobuf.h"
#include "pdu.h"
#include "protocol_engine.h"
#include "stream_protocol.h"

struct stream_protocol_engine {
    protocol_engine_t base;
    pdu_t *encoder_pdu;
    unsigned int encoder_flags;
    uint8_t *read_buffer;
    size_t read_buffer_size;
    pdu_t *decoder_pdu;
    unsigned int decoder_flags;
    uint8_t *write_buffer;
    size_t write_buffer_size;
};

typedef struct stream_protocol_engine stream_protocol_engine_t;

static struct protocol_engine_ops ops;

protocol_engine_t *
stream_protocol_engine_new ()
{
    stream_protocol_engine_t *self =
        (stream_protocol_engine_t *) malloc (sizeof *self);
    if (self) {
        *self = (stream_protocol_engine_t) {
            .base = (protocol_engine_t) {.ops = ops },
            .encoder_flags = ZKERNEL_ENCODER_READY,
            .decoder_flags = ZKERNEL_WRITE_OK,
        };
    }

    return (protocol_engine_t *) self;
}

static int
s_init (protocol_engine_t *base, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return 0;
}

static int
s_encode (protocol_engine_t *base, pdu_t *pdu, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    if (self->encoder_pdu) {
        assert (self->read_buffer_size == 0);
        pdu_destroy (&self->encoder_pdu);
    }

    self->encoder_pdu = pdu;
    self->read_buffer = pdu->pdu_data;
    self->read_buffer_size = pdu->pdu_size;

    self->encoder_flags =
        self->read_buffer_size == 0
            ? ZKERNEL_ENCODER_READY
            : ZKERNEL_READ_OK;

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return 0;
}

static int
s_read (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    const size_t n =
        iobuf_write (iobuf, self->read_buffer, self->read_buffer_size);
    self->read_buffer += n;
    self->read_buffer_size -= n;

    self->encoder_flags =
        self->read_buffer_size == 0
            ? ZKERNEL_ENCODER_READY
            : ZKERNEL_READ_OK;

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return 0;
}

static int
s_read_advance (protocol_engine_t *base, size_t n, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    assert (n <= self->read_buffer_size);
    self->read_buffer += n;
    self->read_buffer_size -= n;

    self->encoder_flags =
        self->read_buffer_size == 0
            ? ZKERNEL_ENCODER_READY
            : ZKERNEL_READ_OK;

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return 0;
}

static pdu_t *
s_decode (protocol_engine_t *base, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    pdu_t *pdu = self->decoder_pdu;
    if (pdu == NULL)
        return NULL;

    self->decoder_pdu = NULL;
    self->decoder_flags = ZKERNEL_WRITE_OK;

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return pdu;
}

static int
s_write (protocol_engine_t *base, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    pdu_t *pdu = self->decoder_pdu;
    if (pdu == NULL) {
        pdu = pdu_new ();
        if (pdu == NULL)
            return -1;
        self->decoder_pdu = pdu;
        self->write_buffer = pdu->pdu_data;
        self->write_buffer_size = pdu->pdu_size;
    }

    const size_t n = iobuf_read (iobuf,
        self->write_buffer, self->write_buffer_size);
    pdu->pdu_size += n;
    self->write_buffer += n;
    self->write_buffer_size -= n;

    self->decoder_flags = 0;
    if (pdu->pdu_size > 0)
        self->decoder_flags |= ZKERNEL_DECODER_READY;
    if (self->write_buffer_size > 0)
        self->decoder_flags |= ZKERNEL_WRITE_OK;

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return 0;
}

static int
s_write_advance (protocol_engine_t *base, size_t n, protocol_engine_info_t *info)
{
    stream_protocol_engine_t *self = (stream_protocol_engine_t *) base;
    assert (self);

    pdu_t *pdu = self->decoder_pdu;
    if (pdu == NULL)
        return -1;
    assert (n <= self->write_buffer_size);

    pdu->pdu_size += n;
    self->write_buffer += n;
    self->write_buffer_size -= n;

    self->decoder_flags = 0;
    if (pdu->pdu_size > 0)
        self->decoder_flags |= ZKERNEL_DECODER_READY;
    if (self->write_buffer_size > 0)
        self->decoder_flags |= ZKERNEL_WRITE_OK;

    *info = (protocol_engine_info_t) {
        .flags = self->encoder_flags | self->decoder_flags,
        .read_buffer = self->read_buffer,
        .read_buffer_size = self->read_buffer_size,
        .write_buffer = self->write_buffer,
        .write_buffer_size = self->write_buffer_size,
    };

    return 0;
}

static void
s_destroy (protocol_engine_t **base_p)
{
    assert (base_p);
    if (*base_p) {
        stream_protocol_engine_t *self = (stream_protocol_engine_t *) *base_p;
        pdu_destroy (&self->encoder_pdu);
        pdu_destroy (&self->decoder_pdu);
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
