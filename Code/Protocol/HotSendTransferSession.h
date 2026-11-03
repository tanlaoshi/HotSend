#ifndef HOT_SEND_TRANSFER_SESSION_H
#define HOT_SEND_TRANSFER_SESSION_H

#include "HotSendProtocol.h"
#include "HotSendTransport.h"

/*
 * Transport 已打开。SerialCommandPath=1：先等壳提示符并以分块方式发 ksend 行
 * （对表 ksend.py 串口）；0：整行 WriteAll（TCP serve）。
 */
int HotSendTransferSession(const HotSendTransferRequest *Request,
                           HotSendTransport *Transport, int SerialCommandPath);

#endif
