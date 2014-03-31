//  Mailbox interface

#include <assert.h>
#include "mailbox.h"

int
mailbox_enqueue (mailbox_t *self, struct msg_t *msg)
{
    assert (self);
    return self->ftab.enqueue (self->object, msg);
}
