#ifndef HOT_SEND_SERIAL_TRANSPORT_H
#define HOT_SEND_SERIAL_TRANSPORT_H

#include "HotSendTransport.h"

/* BaudRate：0 → 115200。成功后 *Out 可用，Close 释放。 */
int HotSendSerialTransportOpen(HotSendTransport *Out, const char *DevicePath,
                               int BaudRate);

int HotSendSerialWaitShellPrompt(HotSendTransport *Transport, double TimeoutSeconds);
int HotSendSerialSendCommandChunked(HotSendTransport *Transport, const char *Command);

#endif
