#ifndef HOT_SEND_SESSION_LINES_H
#define HOT_SEND_SESSION_LINES_H

#include "Transport.h"

#include <stddef.h>
#include <stdint.h>

typedef void (*HotSendLogFunction)(const char *Message, void *UserData);

/* 等到 prefixes 之一开头的行；其它行经 Log 抛出。0=命中，写入 Line。 */
int HotSendWaitForLinePrefix(HotSendTransport *Transport, const char *const *Prefixes,
                             size_t PrefixCount, char *Line, size_t Capacity,
                             double TimeoutSeconds, HotSendLogFunction Log,
                             void *UserData);

/* 等孤立 'A'/'N'；夹杂日志行丢掉。返回 HotSendAckWritten / HotSendAckRetry；-1 失败 */
int HotSendReadBlockAck(HotSendTransport *Transport, double TimeoutSeconds,
                        HotSendLogFunction Log, void *UserData);

/* 读 n×u32 小端 CRC + 末尾 0x0A；n=0 仍吃掉那一字节 */
int HotSendReadBoardCrcTable(HotSendTransport *Transport, uint32_t BlockCount,
                             uint32_t *OutCrcs, double TimeoutSeconds,
                             HotSendLogFunction Log, void *UserData);

/*
 * 从 "KSEND READY size=… block=… n=… [win=…] [via=wired|wifi|serial]" 取字段。
 * size/block/n 缺一返回 -1；win 缺省 1；via 缺省 HotSendViaUnknown。
 */
int HotSendParseReadyFields(const char *Line, uint32_t *OutSize, uint32_t *OutBlock,
                            uint32_t *OutCount, uint32_t *OutWindow, int *OutVia);

#endif
