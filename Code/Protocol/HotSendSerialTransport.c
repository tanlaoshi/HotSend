/*
 * HotSendSerialTransport.c — 串口传输（对表 ksend.py SerialPort）
 */
#include "HotSendSerialTransport.h"

#include "HotSendConstants.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

enum {
    HotSendSerialReceiveCapacity = 65536,
    HotSendDefaultBaudRate = 115200
};

typedef struct HotSendSerialState {
    int FileDescriptor;
    unsigned char Buffer[HotSendSerialReceiveCapacity];
    size_t BufferLength;
} HotSendSerialState;

static double HotSendSerialNow(void) {
    struct timespec TimeSpec;

    clock_gettime(CLOCK_MONOTONIC, &TimeSpec);
    return (double)TimeSpec.tv_sec + (double)TimeSpec.tv_nsec / 1e9;
}

static speed_t HotSendSerialBaudToSpeed(int BaudRate) {
    switch (BaudRate) {
    case 9600:
        return B9600;
    case 115200:
        return B115200;
    case 230400:
        return B230400;
    case 921600:
        return B921600;
    default:
        return (speed_t)0;
    }
}

static int HotSendSerialFill(HotSendSerialState *State, double TimeoutSeconds) {
    fd_set ReadSet;
    struct timeval TimeValue;
    int Ready;
    ssize_t Got;

    FD_ZERO(&ReadSet);
    FD_SET(State->FileDescriptor, &ReadSet);
    if (TimeoutSeconds < 0.0) {
        TimeoutSeconds = 0.0;
    }
    TimeValue.tv_sec = (time_t)TimeoutSeconds;
    TimeValue.tv_usec =
        (suseconds_t)((TimeoutSeconds - (double)TimeValue.tv_sec) * 1e6);
    Ready = select(State->FileDescriptor + 1, &ReadSet, NULL, NULL, &TimeValue);
    if (Ready <= 0) {
        return Ready == 0 ? 0 : -1;
    }
    if (State->BufferLength >= HotSendSerialReceiveCapacity) {
        return -1;
    }
    Got = read(State->FileDescriptor, State->Buffer + State->BufferLength,
               HotSendSerialReceiveCapacity - State->BufferLength);
    if (Got == 0) {
        errno = EIO;
        return -1;
    }
    if (Got < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }
    State->BufferLength += (size_t)Got;
    return 1;
}

static int HotSendSerialWriteAll(HotSendTransport *Self, const void *Data,
                                 size_t Length, double TimeoutSeconds) {
    HotSendSerialState *State = (HotSendSerialState *)Self->State;
    const unsigned char *Cursor = (const unsigned char *)Data;
    size_t Remaining = Length;
    double Deadline = HotSendSerialNow() + TimeoutSeconds;

    while (Remaining > 0) {
        fd_set WriteSet;
        struct timeval TimeValue;
        double Remain;
        int Ready;
        ssize_t Written;

        Remain = Deadline - HotSendSerialNow();
        if (Remain <= 0.0) {
            errno = ETIMEDOUT;
            return -1;
        }
        FD_ZERO(&WriteSet);
        FD_SET(State->FileDescriptor, &WriteSet);
        TimeValue.tv_sec = (time_t)Remain;
        TimeValue.tv_usec = (suseconds_t)((Remain - (double)TimeValue.tv_sec) * 1e6);
        Ready =
            select(State->FileDescriptor + 1, NULL, &WriteSet, NULL, &TimeValue);
        if (Ready <= 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        Written = write(State->FileDescriptor, Cursor, Remaining);
        if (Written < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        }
        Cursor += (size_t)Written;
        Remaining -= (size_t)Written;
    }
    return 0;
}

static int HotSendSerialReadExact(HotSendTransport *Self, void *Data, size_t Length,
                                  double TimeoutSeconds) {
    HotSendSerialState *State = (HotSendSerialState *)Self->State;
    unsigned char *Out = (unsigned char *)Data;
    size_t Need = Length;
    double Deadline = HotSendSerialNow() + TimeoutSeconds;

    while (Need > 0) {
        size_t Take;
        double Remain;

        if (State->BufferLength == 0) {
            Remain = Deadline - HotSendSerialNow();
            if (Remain <= 0.0) {
                errno = ETIMEDOUT;
                return -1;
            }
            if (HotSendSerialFill(State, Remain) <= 0) {
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

static int HotSendSerialReadLine(HotSendTransport *Self, char *Line, size_t Capacity,
                                 double TimeoutSeconds) {
    HotSendSerialState *State = (HotSendSerialState *)Self->State;
    size_t OutLength = 0;
    double Deadline = HotSendSerialNow() + TimeoutSeconds;

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
        Remain = Deadline - HotSendSerialNow();
        if (Remain <= 0.0) {
            errno = ETIMEDOUT;
            return -1;
        }
        if (HotSendSerialFill(State, Remain < 0.2 ? Remain : 0.2) < 0) {
            return -1;
        }
        if (State->BufferLength == 0 && HotSendSerialNow() >= Deadline) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
}

static void HotSendSerialClose(HotSendTransport *Self) {
    HotSendSerialState *State = (HotSendSerialState *)Self->State;

    if (State == NULL) {
        return;
    }
    if (State->FileDescriptor >= 0) {
        close(State->FileDescriptor);
        State->FileDescriptor = -1;
    }
    free(State);
    Self->State = NULL;
}

int HotSendSerialTransportOpen(HotSendTransport *Out, const char *DevicePath,
                               int BaudRate) {
    HotSendSerialState *State;
    struct termios Attributes;
    speed_t Speed;
    int FileDescriptor;

    if (Out == NULL || DevicePath == NULL) {
        return -1;
    }
    if (BaudRate <= 0) {
        BaudRate = HotSendDefaultBaudRate;
    }
    Speed = HotSendSerialBaudToSpeed(BaudRate);
    if (Speed == 0) {
        errno = EINVAL;
        return -1;
    }
    memset(Out, 0, sizeof(*Out));
    State = (HotSendSerialState *)calloc(1, sizeof(*State));
    if (State == NULL) {
        return -1;
    }
    State->FileDescriptor = -1;
    FileDescriptor = open(DevicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (FileDescriptor < 0) {
        free(State);
        return -1;
    }
    if (tcgetattr(FileDescriptor, &Attributes) != 0) {
        close(FileDescriptor);
        free(State);
        return -1;
    }
    cfmakeraw(&Attributes);
    Attributes.c_cflag |= (CS8 | CREAD | CLOCAL);
    Attributes.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS);
    cfsetispeed(&Attributes, Speed);
    cfsetospeed(&Attributes, Speed);
    Attributes.c_cc[VMIN] = 0;
    Attributes.c_cc[VTIME] = 0;
    if (tcsetattr(FileDescriptor, TCSANOW, &Attributes) != 0) {
        close(FileDescriptor);
        free(State);
        return -1;
    }
    (void)tcflush(FileDescriptor, TCIOFLUSH);
    if (ioctl(FileDescriptor, TIOCEXCL) != 0) {
        close(FileDescriptor);
        free(State);
        return -1;
    }
    State->FileDescriptor = FileDescriptor;
    Out->State = State;
    Out->WriteAll = HotSendSerialWriteAll;
    Out->ReadExact = HotSendSerialReadExact;
    Out->ReadLine = HotSendSerialReadLine;
    Out->Close = HotSendSerialClose;
    return 0;
}
