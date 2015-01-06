//  Set class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include "set.h"

struct zset {
    size_t size;
    size_t cursor;
    void *entry [16];
};

zset_t *
zset_new ()
{
    zset_t *self = malloc (sizeof *self);
    if (self)
        *self = (struct zset) { .size = 0 };
    return self;
}

void
zset_destroy (zset_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zset_t *self = *self_p;
        free (self);
        *self_p = NULL;
    }
}

void
zset_add (zset_t *self, void *el)
{
    assert (self);
    assert (self->size < 16);

    for (int i = 0; i < 16; i++)
        if (self->entry [i] == NULL) {
            self->entry [i] = el;
            self->size++;
            break;
        }
}

void
zset_remove (zset_t *self, void *el)
{
    assert (self);
    for (int i = 0; i < 16; i++)
        if (self->entry [i] == el) {
            self->entry [i] == NULL;
            self->size--;
            break;
        }
}

bool
zset_non_empty (zset_t *self)
{
    assert (self);
    return self->size > 0;
}

void *
zset_first (zset_t *self)
{
    assert (self);
    for (int i = 0; i < 16; i++)
        if (self->entry [i] != NULL) {
            self->cursor = i;
            return self->entry [i];
        }
    return NULL;
}

void *
zset_next (zset_t *self)
{
    assert (self);
    for (int i = self->cursor + 1; i < 16; i++)
        if (self->entry [i] != NULL) {
            self->cursor = i;
            return self->entry [i];
        }
    return NULL;
}
