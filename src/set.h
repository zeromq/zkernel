/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZSET_H_INCLUDED__
#define __ZSET_H_INCLUDED__

#include <stdbool.h>

typedef struct zset zset_t;

zset_t *
    zset_new ();

void
    zset_destroy (zset_t **self_p);

void
    zset_add (zset_t *self, void *el);

void
    zset_remove (zset_t *self, void *el);

bool
    zset_non_empty ();

void *
    zset_first (zset_t *self);

void *
    zset_next (zset_t *self);

#endif
