//

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ATOMIC_H_INCLUDED__
#define __ATOMIC_H_INCLUDED__

void
    atomic_ptr_set (void **ptr, void *val);

void *
    atomic_ptr_get (void **ptr);

void *
    atomic_ptr_swap (void **ptr, void *new);

void *
    atomic_ptr_cas (void **ptr, void *old, void *new);

#endif
