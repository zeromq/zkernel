/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stdlib.h>

#include "iobuf.h"
#include "pdu.h"
#include "encoder.h"

struct stream_encoder {
    encoder_t base;
    uint8_t *ptr;
    size_t bytes_left;
    pdu_t *pdu;
};

typedef struct stream_encoder stream_encoder_t;

static struct encoder_ops encoder_ops;

static stream_encoder_t *
s_new ()
{
    stream_encoder_t *self = malloc (sizeof *self);
    if (self)
        *self = (stream_encoder_t) {
            .base = (encoder_t) { .ops = encoder_ops }
        };
    return self;
}

static int
s_encode (encoder_t *base, pdu_t *pdu, encoder_status_t *status)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    if (self->pdu) {
        assert (self->bytes_left == 0);
        pdu_destroy (&self->pdu);
    }

    self->pdu = pdu;
    self->ptr = pdu->pdu_data;
    self->bytes_left = pdu->pdu_size;

    if (self->bytes_left == 0)
        *status = ZKERNEL_ENCODER_READY;
    else
        *status = self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK
                | ZKERNEL_ENCODER_READ_OK;

    return 0;
}

static int
s_read (encoder_t *base, iobuf_t *iobuf, encoder_status_t *status)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    const size_t n =
        iobuf_write (iobuf, self->ptr, self->bytes_left);
    self->ptr += n;
    self->bytes_left -= n;

    if (self->bytes_left == 0)
        *status = ZKERNEL_ENCODER_READY;
    else
        *status = self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK
                | ZKERNEL_ENCODER_READ_OK;

    return 0;
}

static int
s_buffer (encoder_t *base, const void **buffer, size_t *buffer_size)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    if (self->pdu) {
        *buffer = self->ptr;
        *buffer_size = self->bytes_left;
    }
    else {
        *buffer = NULL;
        *buffer_size = 0;
    }

    return 0;
}

static int
s_advance (encoder_t *base, size_t n, encoder_status_t *status)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    assert (n <= self->bytes_left);
    self->ptr += n;
    self->bytes_left -= n;

    *status = self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK;
    if (self->bytes_left == 0)
        *status |= ZKERNEL_ENCODER_READY;
    else
        *status |= ZKERNEL_ENCODER_READ_OK;

    return 0;
}

static encoder_status_t
s_status (encoder_t *base)
{
    stream_encoder_t *self = (stream_encoder_t *) base;
    assert (self);

    if (self->bytes_left == 0)
        return ZKERNEL_ENCODER_READY;
    else
        return (self->bytes_left & ZKERNEL_ENCODER_BUFFER_MASK)
              | ZKERNEL_ENCODER_READ_OK;
}

static void
s_destroy (encoder_t **base_p)
{
    if (*base_p) {
        stream_encoder_t *self = (stream_encoder_t *) *base_p;
        if (self->pdu)
            pdu_destroy (&self->pdu);
        free (self);
        *base_p = NULL;
    }
}

static struct encoder_ops encoder_ops = {
    .encode = s_encode,
    .read = s_read,
    .buffer = s_buffer,
    .advance = s_advance,
    .status = s_status,
    .destroy = s_destroy
};

encoder_t *
stream_encoder_create_encoder ()
{
    return (encoder_t *) s_new ();
}
