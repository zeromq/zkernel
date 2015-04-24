/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.*/

#ifndef __ZKERNEL_H_INCLUDED__
#define __ZKERNEL_H_INCLUDED__

//  Flags
#define ZKERNEL_POLLIN          1
#define ZKERNEL_POLLOUT         2

//  Command ids
#define ZKERNEL_KILL            1
#define ZKERNEL_REGISTER        2
#define ZKERNEL_REMOVE          3
#define ZKERNEL_ACTIVATE        4

//  Event ids
#define ZKERNEL_NEW_SESSION     16
#define ZKERNEL_SESSION_CLOSED  17
#define ZKERNEL_READY_TO_SEND   18

#define ZKERNEL_SESSION         1
#define ZKERNEL_START           2
#define ZKERNEL_START_ACK       3
#define ZKERNEL_START_NAK       4
#define ZKERNEL_STOP_PROXY      5
#define ZKERNEL_PROXY_STOPPED   6

//  Frame ID
#define ZKERNEL_MSG_TYPE_PDU    32

#define ZKERNEL_INPUT_READY     0x01
#define ZKERNEL_OUTPUT_READY    0x02
#define ZKERNEL_IO_ERROR        0x04

#define ZKERNEL_ENCODER_READY   0x01
#define ZKERNEL_READ_OK         0x02
#define ZKERNEL_DECODER_READY   0x04
#define ZKERNEL_WRITE_OK        0x08
#define ZKERNEL_ENGINE_DONE     0x20

#endif
