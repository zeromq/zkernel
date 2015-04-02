//  Protocol engine class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __PROTOCOL_ENGINE_H_INCLUDED__
#define __PROTOCOL_ENGINE_H_INCLUDED__

#include <stdint.h>

#include "iobuf.h"
#include "pdu.h"

typedef struct protocol_engine protocol_engine_t;

typedef struct protocol_engine_info protocol_engine_info_t;

struct protocol_engine_ops {
    int (*init) (protocol_engine_t *self, protocol_engine_info_t *info);
    int (*encode) (protocol_engine_t *self, pdu_t *pdu, protocol_engine_info_t *info);
    int (*read) (protocol_engine_t *self, iobuf_t *iobuf, protocol_engine_info_t *info);
    int (*read_advance) (protocol_engine_t *self, size_t n, protocol_engine_info_t *info);
    pdu_t *(*decode) (protocol_engine_t *self, protocol_engine_info_t *info);
    int (*write) (protocol_engine_t *self, iobuf_t *iobuf, protocol_engine_info_t *info);
    int (*write_advance) (protocol_engine_t *self, size_t n, protocol_engine_info_t *info);
    int (*set_socket_id) (protocol_engine_t *self, const char *socket_id);
    int (*next) (protocol_engine_t **self_p, protocol_engine_info_t *info);
    void (*destroy) (protocol_engine_t **self_p);
};

struct protocol_engine {
    struct protocol_engine_ops ops;
};

struct protocol_engine_info {
    unsigned int flags;
    size_t read_buffer_size;
    void *read_buffer;
    size_t write_buffer_size;
    void *write_buffer;
};

typedef protocol_engine_t *(protocol_engine_constructor_t) ();

inline int
protocol_engine_init (protocol_engine_t *self, protocol_engine_info_t *info)
{
    return self->ops.init (self, info);
}

inline int
protocol_engine_encode (protocol_engine_t *self, pdu_t *pdu, protocol_engine_info_t *info)
{
    return self->ops.encode (self, pdu, info);
}

inline int
protocol_engine_read (protocol_engine_t *self, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    return self->ops.read (self, iobuf, info);
}

inline int
protocol_engine_read_advance (protocol_engine_t *self, size_t n, protocol_engine_info_t *info)
{
    return self->ops.read_advance (self, n, info);
}

inline pdu_t *
protocol_engine_decode (protocol_engine_t *self, protocol_engine_info_t *info)
{
    return self->ops.decode (self, info);
}

inline int
protocol_engine_write (protocol_engine_t *self, iobuf_t *iobuf, protocol_engine_info_t *info)
{
    return self->ops.write (self, iobuf, info);
}

inline int
protocol_engine_write_advance (protocol_engine_t *self, size_t n, protocol_engine_info_t *info)
{
    return self->ops.write_advance (self, n, info);
}

int
    protocol_engine_set_socket_id (protocol_engine_t *self, const char *socket_id);

int
    protocol_engine_next (protocol_engine_t **self_p, protocol_engine_info_t *info);

void
    protocol_engine_destroy (protocol_engine_t **self_p);

#endif
