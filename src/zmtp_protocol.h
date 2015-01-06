//  ZMTP protocol class

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZMTP_PROTOCOL_H_INCLUDED__
#define __ZMTP_PROTOCOL_H_INCLUDED__

#include "protocol.h"

typedef struct zmtp_protocol zmtp_protocol_t;

zmtp_protocol_t *
    zmtp_protocol_new ();

#endif
