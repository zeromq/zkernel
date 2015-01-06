/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

void
atomic_ptr_set (void **ptr, void *val)
{
    *ptr = val;
}

void *
atomic_ptr_get (void **ptr)
{
    return *ptr;
}

void *
atomic_ptr_swap (void **ptr, void *new)
{
    void *old;
    __asm__ volatile (
        "lock xchg %2, %0"
        : "=r" (old), "=m" (*ptr)
        : "m" (*ptr), "0" (new));
    return old;
}

void *
atomic_ptr_cas (void **ptr, void *old, void *new)
{
    void *retval;
    __asm__ volatile (
        "lock cmpxchg %2, %1"
        : "=a" (retval), "=m" (*ptr)
        : "r" (new), "m" (*ptr), "0" (old)
        : "cc");
    return retval;
}
