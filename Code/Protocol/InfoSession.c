/*
 * InfoSession.c — `ksend info`（对表 Tools/ksend.py command_info）
 */
#include "InfoSession.h"

#include "BlockPlan.h"
#include "SerialTransport.h"
#include "SessionLines.h"
#include "TransferSupport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void HotSendInfoLog(const HotSendTransferRequest *Request, const char *Message) {
    if (Request->Log != NULL) {
        Request->Log(Message, Request->UserData);
    }
}

static int HotSendParseInfoFields(const char *Line, uint32_t *OutSize,
                                  uint32_t *OutCrc) {
    const char *Cursor;
    int HaveSize = 0;
    int HaveCrc = 0;
    uint32_t Size = 0;
    uint32_t Crc = 0;

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
        } else if (strncmp(Cursor, "crc=", 4) == 0) {
            Crc = (uint32_t)strtoul(Cursor + 4, NULL, 16);
            HaveCrc = 1;
        }
        while (*Cursor != 0 && *Cursor != ' ') {
            Cursor++;
        }
    }
    if (!HaveSize || !HaveCrc) {
        return -1;
    }
    if (OutSize != NULL) {
        *OutSize = Size;
    }
    if (OutCrc != NULL) {
        *OutCrc = Crc;
    }
    return 0;
}

int HotSendInfoSession(const HotSendTransferRequest *Request,
                       HotSendTransport *Transport, int SerialCommandPath) {
    static const char *const InfoPrefixes[] = {"KSEND INFO ", "KSEND FAIL"};
    static const char Command[] = "ksend info\n";
    char Line[512];
    char Note[192];
    uint32_t BoardSize = 0;
    uint32_t BoardCrc = 0;
    int ExitCode = HotSendExitProtocol;

    if (Request == NULL || Transport == NULL) {
        return HotSendExitLocal;
    }
    if (SerialCommandPath) {
        if (HotSendSerialWaitShellPrompt(Transport,
                                         Request->ReadyTimeoutSeconds) != 0) {
            HotSendInfoLog(Request, "serial: no shell prompt for info");
            return HotSendExitProtocol;
        }
        if (HotSendSerialSendCommandChunked(Transport, Command) != 0) {
            return HotSendExitProtocol;
        }
    } else if (Transport->WriteAll(Transport, Command, strlen(Command), 10.0) != 0) {
        return HotSendExitProtocol;
    }

    if (HotSendWaitForLinePrefix(Transport, InfoPrefixes, 2, Line, sizeof(Line),
                                 Request->ReadyTimeoutSeconds, Request->Log,
                                 Request->UserData) != 0) {
        return HotSendExitProtocol;
    }
    HotSendInfoLog(Request, Line);
    if (strncmp(Line, "KSEND FAIL", 10) == 0) {
        return HotSendExitBoardFail;
    }
    if (HotSendParseInfoFields(Line, &BoardSize, &BoardCrc) != 0) {
        return HotSendExitProtocol;
    }
    if (Request->LocalFilePath == NULL || Request->LocalFilePath[0] == 0) {
        return HotSendExitOk;
    }
    {
        unsigned char *FileData = NULL;
        size_t FileLength = 0;
        uint32_t LocalCrc;

        if (HotSendLoadLocalFile(Request->LocalFilePath, &FileData, &FileLength) != 0) {
            return HotSendExitLocal;
        }
        LocalCrc = HotSendComputeWholeCrc(FileData, FileLength);
        free(FileData);
        if (LocalCrc == BoardCrc && FileLength == (size_t)BoardSize) {
            HotSendInfoLog(Request, "match: local file is identical to the board file");
            ExitCode = HotSendExitOk;
        } else {
            snprintf(Note, sizeof(Note),
                     "diff: board %u B crc=%08x, local %zu B crc=%08x",
                     (unsigned)BoardSize, (unsigned)BoardCrc, FileLength,
                     (unsigned)LocalCrc);
            HotSendInfoLog(Request, Note);
            ExitCode = HotSendExitBoardFail;
        }
    }
    return ExitCode;
}
