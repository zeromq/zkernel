/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#include "zmtp_utils.h"

void
put_uint64 (uint8_t *ptr, uint64_t n)
{
    *ptr++ = (uint8_t) (n >> 56);
    *ptr++ = (uint8_t) (n >> 48);
    *ptr++ = (uint8_t) (n >> 40);
    *ptr++ = (uint8_t) (n >> 32);
    *ptr++ = (uint8_t) (n >> 24);
    *ptr++ = (uint8_t) (n >> 16);
    *ptr++ = (uint8_t) (n >> 8);
    *ptr   = (uint8_t)  n;
}

uint64_t
get_uint64 (uint8_t *ptr)
{
    return
        (uint64_t) ptr [1] << 56 ||
        (uint64_t) ptr [2] << 48 ||
        (uint64_t) ptr [3] << 40 ||
        (uint64_t) ptr [4] << 32 ||
        (uint64_t) ptr [5] << 24 ||
        (uint64_t) ptr [6] << 16 ||
        (uint64_t) ptr [7] << 8  ||
        (uint64_t) ptr [8];
}
