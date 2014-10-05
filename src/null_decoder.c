#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "null_decoder.h"

#define BUFSIZE     80

struct null_decoder {
    char *buffer;
    size_t available;
};

typedef struct null_decoder null_decoder_t;

static null_decoder_t *
s_new ()
{
    null_decoder_t *self = malloc (sizeof *self);
    if (self) {
        char *buffer = (char *) malloc (BUFSIZE);
        if (buffer)
            *self = (null_decoder_t) { .buffer = buffer };
        else {
            free (self);
            self = NULL;
        }
    }
    return self;
}

static void
s_buffer (void *self_, void **ptr, size_t *n)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self);

    *ptr = self->buffer;
    *n = BUFSIZE;
}

static void
s_data_ready (void *self_, size_t n)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self);

    printf ("null_decoder: %z bytes ready\n", n);
    self->available = n;
}

static int
s_decode (void *self_)
{
    null_decoder_t *self = (null_decoder_t *) self_;
    assert (self);

    printf ("null_decoder: decoding %z bytes\n", self->available);
    self->available = 0;
    return 0;
}

static void
s_destroy (void **self_p)
{
    assert (self_p);
    if (*self_p) {
        null_decoder_t *self = (null_decoder_t *) *self_p;
        free (self->buffer);
        free (self);
        *self_p = NULL;
    }
}

msg_decoder_t *
null_decoder_create_decoder ()
{
    static struct msg_decoder_ops ops = {
        .buffer = s_buffer,
        .data_ready = s_data_ready,
        .decode = s_decode,
        .destroy = s_destroy
    };

    msg_decoder_t *self = (msg_decoder_t *) malloc (sizeof *self);
    if (self) {
        *self = (msg_decoder_t) { .object = s_new (), .ops = ops };
        if (self->object == NULL) {
            free (self);
            self = NULL;
        }
    }

    return self;
}
