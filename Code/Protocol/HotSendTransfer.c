/*
 * HotSendTransfer.c — TCP / 串口入口（会话见 HotSendTransferSession.c）
 */
#include "HotSendProtocol.h"

#include "HotSendSerialTransport.h"
#include "HotSendTcpTransport.h"
#include "HotSendTransferSession.h"

#include <stdio.h>
#include <string.h>

static void HotSendTransferLog(const HotSendTransferRequest *Request,
                               const char *Message) {
    if (Request->Log != NULL) {
        Request->Log(Message, Request->UserData);
    }
}

void HotSendTransferRequestInit(HotSendTransferRequest *Request) {
    if (Request == NULL) {
        return;
    }
    memset(Request, 0, sizeof(*Request));
    Request->TcpPort = HotSendDefaultTcpPort;
    Request->MaximumSegmentSize = HotSendTcpMssDefault;
    Request->BaudRate = 115200;
    Request->BoardTargetPath = HOT_SEND_DEFAULT_TARGET_PATH;
    Request->ReadyTimeoutSeconds = 180.0;
    Request->AckTimeoutSeconds = 30.0;
    Request->DataTimeoutSeconds = 30.0;
    Request->VerifyTimeoutSeconds = 300.0;
}

int HotSendTransferOverTcp(const HotSendTransferRequest *Request) {
    HotSendTransport Transport;
    char Note[96];
    int TcpPort;
    int Mss;
    int ExitCode;
    int Opened = 0;

    if (Request == NULL || Request->Host == NULL || Request->LocalFilePath == NULL) {
        return HotSendExitLocal;
    }
    TcpPort = Request->TcpPort > 0 ? Request->TcpPort : (int)HotSendDefaultTcpPort;
    Mss = Request->MaximumSegmentSize;
    if (Mss == 0) {
        Mss = HotSendTcpMssDefault;
    }
    snprintf(Note, sizeof(Note), "tcp: %s:%d", Request->Host, TcpPort);
    HotSendTransferLog(Request, Note);
    if (HotSendTcpTransportOpen(&Transport, Request->Host, TcpPort, Mss < 0 ? 0 : Mss) !=
        0) {
        return HotSendExitProtocol;
    }
    Opened = 1;
    ExitCode = HotSendTransferSession(Request, &Transport, 0);
    if (Opened) {
        Transport.Close(&Transport);
    }
    return ExitCode;
}

int HotSendTransferOverSerial(const HotSendTransferRequest *Request) {
    HotSendTransport Transport;
    char Note[128];
    int BaudRate;
    int ExitCode;
    int Opened = 0;

    if (Request == NULL || Request->SerialDevicePath == NULL ||
        Request->LocalFilePath == NULL) {
        return HotSendExitLocal;
    }
    BaudRate = Request->BaudRate > 0 ? Request->BaudRate : 115200;
    snprintf(Note, sizeof(Note), "serial: %s %d 8N1", Request->SerialDevicePath,
             BaudRate);
    HotSendTransferLog(Request, Note);
    if (HotSendSerialTransportOpen(&Transport, Request->SerialDevicePath, BaudRate) !=
        0) {
        return HotSendExitProtocol;
    }
    Opened = 1;
    ExitCode = HotSendTransferSession(Request, &Transport, 1);
    if (Opened) {
        Transport.Close(&Transport);
    }
    return ExitCode;
}
