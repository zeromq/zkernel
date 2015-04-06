/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZMTP_V1_EXCHANGE_ID_H_INCLUDED__
#define __ZMTP_V1_EXCHANGE_ID_H_INCLUDED__

#include <stddef.h>

#include "protocol_engine.h"

protocol_engine_t *
    zmtp_v1_exchange_id_new_protocol_engine (
        const char *id, size_t peer_id_length);

#endif
