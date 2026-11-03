#ifndef HOT_SEND_INFO_SESSION_H
#define HOT_SEND_INFO_SESSION_H

#include "Protocol.h"
#include "Transport.h"

/* Transport 已打开。发 ksend info；可选对照 LocalFilePath。 */
int HotSendInfoSession(const HotSendTransferRequest *Request,
                       HotSendTransport *Transport, int SerialCommandPath);

#endif
