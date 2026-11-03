/*
 * HotSendSessionLines.c — READY/ACK/CRC 表行协议（对表 ksend.py）
 */
#include "HotSendSessionLines.h"

#include "HotSendConstants.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double HotSendSessionNow(void) {
    struct timespec TimeSpec;

    clock_gettime(CLOCK_MONOTONIC, &TimeSpec);
    return (double)TimeSpec.tv_sec + (double)TimeSpec.tv_nsec / 1e9;
}

static void HotSendSessionLog(HotSendLogFunction Log, void *UserData,
                              const char *Message) {
    if (Log != NULL) {
        Log(Message, UserData);
    }
}

static int HotSendLineStartsWith(const char *Line, const char *Prefix) {
    size_t Length;

    if (Line == NULL || Prefix == NULL) {
        return 0;
    }
    Length = strlen(Prefix);
    return strncmp(Line, Prefix, Length) == 0;
}

int HotSendWaitForLinePrefix(HotSendTransport *Transport, const char *const *Prefixes,
                             size_t PrefixCount, char *Line, size_t Capacity,
                             double TimeoutSeconds, HotSendLogFunction Log,
                             void *UserData) {
    double Deadline;
    double NextHeartbeat;
    char Note[160];
    size_t Index;

    if (Transport == NULL || Prefixes == NULL || PrefixCount == 0 || Line == NULL) {
        return -1;
    }
    Deadline = HotSendSessionNow() + TimeoutSeconds;
    NextHeartbeat = HotSendSessionNow() + 5.0;
    while (HotSendSessionNow() < Deadline) {
        double Remain = Deadline - HotSendSessionNow();
        int Status;

        if (HotSendSessionNow() >= NextHeartbeat) {
            snprintf(Note, sizeof(Note), "waiting for board line … %ds (timeout %ds)",
                     (int)(TimeoutSeconds - Remain), (int)TimeoutSeconds);
            HotSendSessionLog(Log, UserData, Note);
            NextHeartbeat = HotSendSessionNow() + 5.0;
        }
        Status = Transport->ReadLine(Transport, Line, Capacity, Remain < 1.0 ? Remain : 1.0);
        if (Status != 0) {
            if (errno == ETIMEDOUT) {
                continue;
            }
            return -1;
        }
        for (Index = 0; Index < PrefixCount; Index++) {
            if (HotSendLineStartsWith(Line, Prefixes[Index])) {
                return 0;
            }
        }
        if (Line[0] != 0) {
            snprintf(Note, sizeof(Note), "board: %s", Line);
            HotSendSessionLog(Log, UserData, Note);
        }
    }
    errno = ETIMEDOUT;
    return -1;
}

int HotSendReadBlockAck(HotSendTransport *Transport, double TimeoutSeconds,
                        HotSendLogFunction Log, void *UserData) {
    double Deadline;
    unsigned char Line[512];
    size_t LineLength = 0;

    if (Transport == NULL) {
        return -1;
    }
    Deadline = HotSendSessionNow() + TimeoutSeconds;
    while (HotSendSessionNow() < Deadline) {
        unsigned char Byte;
        double Remain = Deadline - HotSendSessionNow();
        char Note[128];

        if (Transport->ReadExact(Transport, &Byte, 1, Remain) != 0) {
            return -1;
        }
        if ((Byte == HotSendAckWritten || Byte == HotSendAckRetry) && LineLength == 0) {
            return (int)Byte;
        }
        if (Byte == (unsigned char)'\n') {
            if (LineLength > 0) {
                char Preview[96];
                size_t PreviewLength =
                    LineLength < sizeof(Preview) - 1 ? LineLength : sizeof(Preview) - 1;

                memcpy(Preview, Line, PreviewLength);
                Preview[PreviewLength] = 0;
                snprintf(Note, sizeof(Note), "ignored log before ack: %s", Preview);
                HotSendSessionLog(Log, UserData, Note);
            }
            LineLength = 0;
            continue;
        }
        if (Byte == (unsigned char)'\r') {
            continue;
        }
        if (LineLength + 1 >= sizeof(Line)) {
            errno = EPROTO;
            return -1;
        }
        Line[LineLength++] = Byte;
    }
    errno = ETIMEDOUT;
    return -1;
}

int HotSendReadBoardCrcTable(HotSendTransport *Transport, uint32_t BlockCount,
                             uint32_t *OutCrcs, double TimeoutSeconds,
                             HotSendLogFunction Log, void *UserData) {
    uint32_t Index;
    unsigned char Terminator;
    char Note[96];

    if (Transport == NULL) {
        return -1;
    }
    if (BlockCount > 0 && OutCrcs == NULL) {
        return -1;
    }
    if (BlockCount > 0) {
        snprintf(Note, sizeof(Note), "reading board CRC table: %u blocks (%u B)",
                 (unsigned)BlockCount, (unsigned)(BlockCount * 4u));
        HotSendSessionLog(Log, UserData, Note);
    }
    for (Index = 0; Index < BlockCount; Index++) {
        unsigned char Word[4];

        if (Transport->ReadExact(Transport, Word, 4, TimeoutSeconds) != 0) {
            return -1;
        }
        if (Word[0] == (unsigned char)'K' && Index == 0) {
            /* 可能误入 KSEND FAIL 文本；交给上层超时/失败路径 */
        }
        OutCrcs[Index] = (uint32_t)Word[0] | ((uint32_t)Word[1] << 8) |
                         ((uint32_t)Word[2] << 16) | ((uint32_t)Word[3] << 24);
    }
    if (Transport->ReadExact(Transport, &Terminator, 1, 5.0) != 0) {
        return -1;
    }
    if (Terminator != (unsigned char)'\n') {
        errno = EPROTO;
        return -1;
    }
    return 0;
}

int HotSendParseReadyFields(const char *Line, uint32_t *OutSize, uint32_t *OutBlock,
                            uint32_t *OutCount) {
    const char *Cursor;
    int HaveSize = 0;
    int HaveBlock = 0;
    int HaveCount = 0;
    uint32_t Size = 0;
    uint32_t Block = 0;
    uint32_t Count = 0;

    if (Line == NULL) {
        return -1;
    }
    Cursor = Line;
    while (*Cursor != 0) {
        while (*Cursor == ' ') {
            Cursor++;
        }
        if (*Cursor == 0) {
            break;
        }
        if (strncmp(Cursor, "size=", 5) == 0) {
            Size = (uint32_t)strtoul(Cursor + 5, NULL, 10);
            HaveSize = 1;
        } else if (strncmp(Cursor, "block=", 6) == 0) {
            Block = (uint32_t)strtoul(Cursor + 6, NULL, 10);
            HaveBlock = 1;
        } else if (strncmp(Cursor, "n=", 2) == 0) {
            Count = (uint32_t)strtoul(Cursor + 2, NULL, 10);
            HaveCount = 1;
        }
        while (*Cursor != 0 && *Cursor != ' ') {
            Cursor++;
        }
    }
    if (!HaveSize || !HaveBlock || !HaveCount) {
        return -1;
    }
    if (OutSize) {
        *OutSize = Size;
    }
    if (OutBlock) {
        *OutBlock = Block;
    }
    if (OutCount) {
        *OutCount = Count;
    }
    return 0;
}
