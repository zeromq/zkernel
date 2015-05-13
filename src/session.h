//  Session base class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __SESSION_H_INCLUDED__
#define __SESSION_H_INCLUDED__

#include <stddef.h>

#include "io_object.h"

typedef struct session session_t;

struct session_ops {
    void (*set_session_id) (session_t *self, unsigned long session_id);
    void (*destroy) (session_t **self_p);
};

struct session {
    io_object_t base;
    struct session_ops ops;
};

void
    session_set_session_id (session_t *self, unsigned long session_id);

void
    session_destroy (session_t **self_p);

#endif
