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

#define ZKERNEL_INPUT_READY     0x01
#define ZKERNEL_OUTPUT_READY    0x02
#define ZKERNEL_IO_ERROR        0x04

#endif
