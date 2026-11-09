/*
 * TcpTransport.c — TCP 到板端 ksend serve
 */
#include "TcpTransport.h"

#include "Constants.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

enum { HotSendTcpReceiveCapacity = 65536 };

typedef struct HotSendTcpState {
    int SocketDescriptor;
    unsigned char Buffer[HotSendTcpReceiveCapacity];
    size_t BufferLength;
} HotSendTcpState;

static double HotSendNowSeconds(void) {
    struct timespec TimeSpec;

    clock_gettime(CLOCK_MONOTONIC, &TimeSpec);
    return (double)TimeSpec.tv_sec + (double)TimeSpec.tv_nsec / 1e9;
}

static int HotSendTcpFill(HotSendTcpState *State, double TimeoutSeconds) {
    fd_set ReadSet;
    struct timeval TimeValue;
    int Ready;
    ssize_t Got;

    if (State->SocketDescriptor < 0) {
        return -1;
    }
    FD_ZERO(&ReadSet);
    FD_SET(State->SocketDescriptor, &ReadSet);
    if (TimeoutSeconds < 0.0) {
        TimeoutSeconds = 0.0;
    }
    TimeValue.tv_sec = (time_t)TimeoutSeconds;
    TimeValue.tv_usec =
        (suseconds_t)((TimeoutSeconds - (double)TimeValue.tv_sec) * 1e6);
    Ready = select(State->SocketDescriptor + 1, &ReadSet, NULL, NULL, &TimeValue);
    if (Ready <= 0) {
        return Ready == 0 ? 0 : -1;
    }
    if (State->BufferLength >= HotSendTcpReceiveCapacity) {
        return -1;
    }
    Got = recv(State->SocketDescriptor, State->Buffer + State->BufferLength,
               HotSendTcpReceiveCapacity - State->BufferLength, 0);
    if (Got == 0) {
        errno = ECONNRESET;
        return -1;
    }
    if (Got < 0) {
        return -1;
    }
    State->BufferLength += (size_t)Got;
    return 1;
}

static int HotSendTcpWriteAll(HotSendTransport *Self, const void *Data,
                              size_t Length, double TimeoutSeconds) {
    HotSendTcpState *State = (HotSendTcpState *)Self->State;
    const unsigned char *Cursor = (const unsigned char *)Data;
    size_t Remaining = Length;
    double Deadline = HotSendNowSeconds() + TimeoutSeconds;

    while (Remaining > 0) {
        fd_set WriteSet;
        struct timeval TimeValue;
        double Remain;
        int Ready;
        ssize_t Written;

        Remain = Deadline - HotSendNowSeconds();
        if (Remain <= 0.0) {
            errno = ETIMEDOUT;
            return -1;
        }
        FD_ZERO(&WriteSet);
        FD_SET(State->SocketDescriptor, &WriteSet);
        TimeValue.tv_sec = (time_t)Remain;
        TimeValue.tv_usec = (suseconds_t)((Remain - (double)TimeValue.tv_sec) * 1e6);
        Ready =
            select(State->SocketDescriptor + 1, NULL, &WriteSet, NULL, &TimeValue);
        if (Ready <= 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        Written = send(State->SocketDescriptor, Cursor, Remaining, MSG_NOSIGNAL);
        if (Written < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            return -1;
        }
        if (Written == 0) {
            errno = ECONNRESET;
            return -1;
        }
        Cursor += (size_t)Written;
        Remaining -= (size_t)Written;
    }
    return 0;
}

static int HotSendTcpReadExact(HotSendTransport *Self, void *Data, size_t Length,
                               double TimeoutSeconds) {
    HotSendTcpState *State = (HotSendTcpState *)Self->State;
    unsigned char *Out = (unsigned char *)Data;
    size_t Need = Length;
    double Deadline = HotSendNowSeconds() + TimeoutSeconds;

    while (Need > 0) {
        size_t Take;
        double Remain;

        if (State->BufferLength == 0) {
            Remain = Deadline - HotSendNowSeconds();
            if (Remain <= 0.0) {
                errno = ETIMEDOUT;
                return -1;
            }
            if (HotSendTcpFill(State, Remain) <= 0) {
                if (errno == 0) {
                    errno = ETIMEDOUT;
                }
                return -1;
            }
        }
        Take = State->BufferLength < Need ? State->BufferLength : Need;
        memcpy(Out, State->Buffer, Take);
        memmove(State->Buffer, State->Buffer + Take, State->BufferLength - Take);
        State->BufferLength -= Take;
        Out += Take;
        Need -= Take;
    }
    return 0;
}

static int HotSendTcpReadLine(HotSendTransport *Self, char *Line, size_t Capacity,
                              double TimeoutSeconds) {
    HotSendTcpState *State = (HotSendTcpState *)Self->State;
    size_t OutLength = 0;
    double Deadline = HotSendNowSeconds() + TimeoutSeconds;

    if (Capacity < 2) {
        return -1;
    }
    for (;;) {
        size_t Index;
        double Remain;

        for (Index = 0; Index < State->BufferLength; Index++) {
            unsigned char Byte = State->Buffer[Index];

            memmove(State->Buffer, State->Buffer + 1, State->BufferLength - 1);
            State->BufferLength--;
            if (Byte == (unsigned char)'\r') {
                /* 须再从下标 0 读：memmove 后 \\n 已到队首，禁 Index++ 跳过 */
                Index = (size_t)-1;
                continue;
            }
            if (Byte == (unsigned char)'\n') {
                Line[OutLength] = 0;
                return 0;
            }
            if (OutLength + 1 >= Capacity) {
                errno = EMSGSIZE;
                return -1;
            }
            Line[OutLength++] = (char)Byte;
            Index = (size_t)-1;
        }
        Remain = Deadline - HotSendNowSeconds();
        if (Remain <= 0.0) {
            /*
             * 超时不得丢掉已吃进 Line 的字节：Wi‑Fi 首段常只有 "KSEN"，
             * WaitForLinePrefix 1s 切片超时后再读会变成 board: D READY /
             * DY size=（Duo 2026-11-09 真机）。
             */
            if (OutLength > 0) {
                if (OutLength + State->BufferLength > (size_t)HotSendTcpReceiveCapacity) {
                    errno = EMSGSIZE;
                    return -1;
                }
                memmove(State->Buffer + OutLength, State->Buffer, State->BufferLength);
                memcpy(State->Buffer, Line, OutLength);
                State->BufferLength += OutLength;
            }
            errno = ETIMEDOUT;
            return -1;
        }
        if (HotSendTcpFill(State, Remain < 0.2 ? Remain : 0.2) < 0) {
            /*
             * 对端在发出 KSEND OK 后关套接字；半行无 '\\n' 时仍认判据
             * （板串口已 OK、TCP 尾包偶发无换行+FIN）。
             */
            if (errno == ECONNRESET && OutLength >= 8 &&
                strncmp(Line, "KSEND OK", 8) == 0) {
                Line[OutLength] = 0;
                return 0;
            }
            if (errno == ECONNRESET && OutLength >= 10 &&
                strncmp(Line, "KSEND FAIL", 10) == 0) {
                Line[OutLength] = 0;
                return 0;
            }
            return -1;
        }
        if (State->BufferLength == 0 && HotSendNowSeconds() >= Deadline) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
}

static void HotSendTcpClose(HotSendTransport *Self) {
    HotSendTcpState *State = (HotSendTcpState *)Self->State;

    if (State == NULL) {
        return;
    }
    if (State->SocketDescriptor >= 0) {
        close(State->SocketDescriptor);
        State->SocketDescriptor = -1;
    }
    free(State);
    Self->State = NULL;
}

int HotSendTcpTransportOpen(HotSendTransport *Out, const char *Host, int TcpPort,
                            int MaximumSegmentSize) {
    HotSendTcpState *State;
    struct sockaddr_in Address;
    int SocketDescriptor;
    int One = 1;

    if (Out == NULL || Host == NULL || TcpPort <= 0 || TcpPort > 65535) {
        return -1;
    }
    memset(Out, 0, sizeof(*Out));
    State = (HotSendTcpState *)calloc(1, sizeof(*State));
    if (State == NULL) {
        return -1;
    }
    State->SocketDescriptor = -1;
    SocketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (SocketDescriptor < 0) {
        free(State);
        return -1;
    }
    if (MaximumSegmentSize > 0) {
        (void)setsockopt(SocketDescriptor, IPPROTO_TCP, TCP_MAXSEG,
                         &MaximumSegmentSize, sizeof(MaximumSegmentSize));
    }
    (void)setsockopt(SocketDescriptor, IPPROTO_TCP, TCP_NODELAY, &One, sizeof(One));
    memset(&Address, 0, sizeof(Address));
    Address.sin_family = AF_INET;
    Address.sin_port = htons((uint16_t)TcpPort);
    if (inet_pton(AF_INET, Host, &Address.sin_addr) != 1) {
        close(SocketDescriptor);
        free(State);
        errno = EINVAL;
        return -1;
    }
    if (connect(SocketDescriptor, (struct sockaddr *)&Address, sizeof(Address)) !=
        0) {
        close(SocketDescriptor);
        free(State);
        return -1;
    }
    State->SocketDescriptor = SocketDescriptor;
    Out->State = State;
    Out->WriteAll = HotSendTcpWriteAll;
    Out->ReadExact = HotSendTcpReadExact;
    Out->ReadLine = HotSendTcpReadLine;
    Out->Close = HotSendTcpClose;
    return 0;
}
