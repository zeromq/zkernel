#include "io_object.h"

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

extern inline int
io_object_init (io_object_t *self, int *fd, uint32_t *timer_interval);

extern inline int
io_object_event (io_object_t *self, uint32_t flags, int *fd, uint32_t *timer_interval);

extern inline int
io_object_message (io_object_t *self, msg_t *msg);

extern inline int
io_object_timeout (io_object_t *self, int *fd, uint32_t *timer_interval);
