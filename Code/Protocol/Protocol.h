/*
 * Protocol.h — 公开 API（TCP / 串口差量热传）
 */
#ifndef HOT_SEND_PROTOCOL_H
#define HOT_SEND_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include "Constants.h"

typedef void (*HotSendLogFunction)(const char *Message, void *UserData);
typedef void (*HotSendProgressFunction)(uint32_t Position, uint32_t Total,
                                        uint32_t SentBytes, void *UserData);

typedef struct HotSendTransferRequest {
    const char *Host;
    int TcpPort; /* 0 → 26992 */
    int MaximumSegmentSize; /* 0 → 1200；<0 不设 TCP_MAXSEG */
    const char *SerialDevicePath; /* 非 NULL 则走串口（与 Host 互斥） */
    int BaudRate; /* 0 → 115200 */
    const char *LocalFilePath; /* InfoOnly 时可 NULL（只查板） */
    const char *BoardTargetPath; /* NULL → BLOCKS:Kernel.elf */
    int NoReboot;
    int NoBackup;
    int InfoOnly; /* 发 ksend info；与 DryRun 互斥 */
    int DryRun; /* 握手+差量表后 abort，不写盘 */
    int PaceMilliseconds; /* >0：每块 ACK 后睡 N ms，且每 16 块再加长歇（对表 ksend.py） */
    int SteadyThrottle; /* 默认关；若 pace=0 则当作 pace-ms=500（Wi‑Fi/xHCI 应急） */
    double ReadyTimeoutSeconds;
    double AckTimeoutSeconds;
    double DataTimeoutSeconds;
    double VerifyTimeoutSeconds;
    HotSendLogFunction Log;
    HotSendProgressFunction Progress;
    void *UserData;
    volatile int *CancelFlag;
} HotSendTransferRequest;

void HotSendTransferRequestInit(HotSendTransferRequest *Request);

int HotSendTransferOverTcp(const HotSendTransferRequest *Request);
int HotSendTransferOverSerial(const HotSendTransferRequest *Request);

#endif
