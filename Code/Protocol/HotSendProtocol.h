/*
 * HotSendProtocol.h — 公开 API（TCP 差量热传）
 */
#ifndef HOT_SEND_PROTOCOL_H
#define HOT_SEND_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include "HotSendConstants.h"

typedef void (*HotSendLogFunction)(const char *Message, void *UserData);
typedef void (*HotSendProgressFunction)(uint32_t Position, uint32_t Total,
                                        uint32_t SentBytes, void *UserData);

typedef struct HotSendTransferRequest {
    const char *Host;
    int TcpPort; /* 0 → 26992 */
    int MaximumSegmentSize; /* 0 → 1200；<0 不设 TCP_MAXSEG */
    const char *LocalFilePath;
    const char *BoardTargetPath; /* NULL → BLOCKS:Kernel.elf */
    int NoReboot;
    int NoBackup;
    double ReadyTimeoutSeconds;
    double AckTimeoutSeconds;
    double DataTimeoutSeconds;
    double VerifyTimeoutSeconds;
    HotSendLogFunction Log;
    HotSendProgressFunction Progress;
    void *UserData;
} HotSendTransferRequest;

void HotSendTransferRequestInit(HotSendTransferRequest *Request);

/* 返回 HotSendExitCode */
int HotSendTransferOverTcp(const HotSendTransferRequest *Request);

#endif
