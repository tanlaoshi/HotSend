#ifndef HOT_SEND_TCP_TRANSPORT_H
#define HOT_SEND_TCP_TRANSPORT_H

#include "Transport.h"

/*
 * 0=成功；非 0=本地/连接失败。成功后 *Out 可读写，用完 Close。
 * MaximumSegmentSize：0=不设 TCP_MAXSEG；常用 1200（对表 ksend.py）。
 */
int HotSendTcpTransportOpen(HotSendTransport *Out, const char *Host,
                            int TcpPort, int MaximumSegmentSize);

#endif
