//  Original ZeroMQ wire codec (aka ZMTP v1)

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZMTP_V1_FRAME_CODEC_INCLUDED__
#define __ZMTP_V1_FRAME_CODEC_INCLUDED__

#include "protocol_engine.h"

protocol_engine_t *
    zmtp_v1_frame_codec_new_protocol_engine ();

#endif
