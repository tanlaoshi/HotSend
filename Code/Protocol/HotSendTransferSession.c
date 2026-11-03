/*
 * HotSendTransferSession.c — READY→差量→trailer→判据（TCP/串口共用）
 */
#include "HotSendTransferSession.h"

#include "HotSendBlockPlan.h"
#include "HotSendConstants.h"
#include "HotSendSerialTransport.h"
#include "HotSendSessionLines.h"
#include "HotSendTransferSupport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void HotSendSessionSleepSeconds(double Seconds) {
    struct timespec Request;

    if (Seconds <= 0.0) {
        return;
    }
    Request.tv_sec = (time_t)Seconds;
    Request.tv_nsec = (long)((Seconds - (double)Request.tv_sec) * 1e9);
    if (Request.tv_nsec < 0) {
        Request.tv_nsec = 0;
    }
    (void)nanosleep(&Request, NULL);
}

static void HotSendSessionLog(const HotSendTransferRequest *Request,
                              const char *Message) {
    if (Request->Log != NULL) {
        Request->Log(Message, Request->UserData);
    }
}

int HotSendTransferSession(const HotSendTransferRequest *Request,
                           HotSendTransport *Transport, int SerialCommandPath) {
    unsigned char *FileData = NULL;
    size_t FileLength = 0;
    uint32_t *LocalCrcs = NULL;
    uint32_t *BoardCrcs = NULL;
    uint32_t *Plan = NULL;
    uint32_t LocalCount = 0;
    uint32_t BoardCount = 0;
    uint32_t PlanCount = 0;
    uint32_t WholeCrc;
    uint32_t BoardSize = 0;
    uint32_t BoardBlock = 0;
    const char *Target;
    char Trigger[160];
    char Line[512];
    char Note[192];
    int ExitCode = HotSendExitProtocol;
    uint32_t Position;
    static const char *const ReadyPrefixes[] = {"KSEND READY ", "KSEND FAIL"};

    Target = Request->BoardTargetPath != NULL ? Request->BoardTargetPath
                                              : HOT_SEND_DEFAULT_TARGET_PATH;
    if (HotSendValidateBoardTargetPath(Target) != 0) {
        return HotSendExitLocal;
    }
    if (HotSendLoadLocalFile(Request->LocalFilePath, &FileData, &FileLength) != 0) {
        return HotSendExitLocal;
    }
    LocalCrcs = (uint32_t *)calloc(HotSendMaxBlocks, sizeof(uint32_t));
    Plan = (uint32_t *)calloc(HotSendMaxBlocks, sizeof(uint32_t));
    if (LocalCrcs == NULL || Plan == NULL) {
        ExitCode = HotSendExitLocal;
        goto Done;
    }
    if (HotSendComputeBlockCrcs(FileData, FileLength, LocalCrcs, &LocalCount) != 0) {
        ExitCode = HotSendExitLocal;
        goto Done;
    }
    WholeCrc = HotSendComputeWholeCrc(FileData, FileLength);
    snprintf(Note, sizeof(Note), "file: %s", Request->LocalFilePath);
    HotSendSessionLog(Request, Note);
    snprintf(Note, sizeof(Note), "target: %s", Target);
    HotSendSessionLog(Request, Note);
    snprintf(Note, sizeof(Note), "local: %zu B, %u blocks, crc=%08x", FileLength,
             (unsigned)LocalCount, (unsigned)WholeCrc);
    HotSendSessionLog(Request, Note);

    if (HotSendBuildTriggerCommand(Trigger, sizeof(Trigger), Target, Request->NoReboot,
                                   Request->NoBackup) != 0) {
        ExitCode = HotSendExitLocal;
        goto Done;
    }
    if (SerialCommandPath) {
        if (HotSendSerialWaitShellPrompt(Transport, 15.0) != 0) {
            HotSendSessionLog(Request,
                              "serial: no shell prompt (need Blocks> ; exit ksend "
                              "serve; close picocom)");
            ExitCode = HotSendExitProtocol;
            goto Done;
        }
        if (HotSendSerialSendCommandChunked(Transport, Trigger) != 0) {
            ExitCode = HotSendExitProtocol;
            goto Done;
        }
    } else if (Transport->WriteAll(Transport, Trigger, strlen(Trigger), 10.0) != 0) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }

    if (HotSendWaitForLinePrefix(Transport, ReadyPrefixes, 2, Line, sizeof(Line),
                                 Request->ReadyTimeoutSeconds, Request->Log,
                                 Request->UserData) != 0) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }
    HotSendSessionLog(Request, Line);
    if (strncmp(Line, "KSEND FAIL", 10) == 0) {
        ExitCode = HotSendExitBoardFail;
        goto Done;
    }
    if (HotSendParseReadyFields(Line, &BoardSize, &BoardBlock, &BoardCount) != 0) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }
    if (BoardBlock != HotSendBlockBytes) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }
    if (BoardCount != (BoardSize + HotSendBlockBytes - 1) / HotSendBlockBytes &&
        !(BoardSize == 0 && BoardCount == 0)) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }
    if (BoardCount > HotSendMaxBlocks) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }
    BoardCrcs = (uint32_t *)calloc(BoardCount > 0 ? BoardCount : 1, sizeof(uint32_t));
    if (BoardCrcs == NULL) {
        ExitCode = HotSendExitLocal;
        goto Done;
    }
    if (HotSendReadBoardCrcTable(Transport, BoardCount, BoardCrcs,
                                 Request->DataTimeoutSeconds, Request->Log,
                                 Request->UserData) != 0) {
        ExitCode = HotSendExitProtocol;
        goto Done;
    }
    PlanCount =
        HotSendBuildDiffPlan(LocalCrcs, LocalCount, BoardCrcs, BoardCount, Plan);
    snprintf(Note, sizeof(Note), "diff: %u/%u blocks to send", (unsigned)PlanCount,
             (unsigned)LocalCount);
    HotSendSessionLog(Request, Note);

    if (Request->DryRun) {
        static const char *const AbortPrefixes[] = {"KSEND ABORT", "KSEND FAIL"};

        HotSendSessionLog(Request, "dry-run: nothing written; aborting board transfer");
        if (HotSendSendAbortFrame(Transport) != 0) {
            ExitCode = HotSendExitProtocol;
            goto Done;
        }
        Line[0] = 0;
        if (HotSendWaitForLinePrefix(Transport, AbortPrefixes, 2, Line, sizeof(Line),
                                     Request->AckTimeoutSeconds, Request->Log,
                                     Request->UserData) == 0) {
            HotSendSessionLog(Request, Line);
        } else {
            HotSendSessionLog(Request,
                              "board did not confirm abort (it will time out alone)");
        }
        ExitCode = HotSendExitOk;
        goto Done;
    }

    for (Position = 0; Position < PlanCount; Position++) {
        uint32_t BlockIndex = Plan[Position];
        size_t Offset = (size_t)BlockIndex * HotSendBlockBytes;
        uint32_t Length = (uint32_t)(FileLength - Offset);
        unsigned char Header[16];
        int Attempt = 0;
        int Ack;
        uint32_t SentBytes = 0;
        uint32_t Scan;

        if (Request->CancelFlag != NULL && *Request->CancelFlag != 0) {
            HotSendSessionLog(Request, "cancel: aborting transfer");
            (void)HotSendSendAbortFrame(Transport);
            ExitCode = HotSendExitProtocol;
            goto Done;
        }
        if (Length > HotSendBlockBytes) {
            Length = HotSendBlockBytes;
        }
        HotSendPutUint32Little(Header + 0, HotSendMagic);
        HotSendPutUint32Little(Header + 4, BlockIndex);
        HotSendPutUint32Little(Header + 8, Length);
        HotSendPutUint32Little(Header + 12, LocalCrcs[BlockIndex]);
        for (;;) {
            if (Transport->WriteAll(Transport, Header, sizeof(Header),
                                    Request->AckTimeoutSeconds) != 0 ||
                Transport->WriteAll(Transport, FileData + Offset, Length,
                                    Request->AckTimeoutSeconds) != 0) {
                ExitCode = HotSendExitProtocol;
                goto Done;
            }
            Ack = HotSendReadBlockAck(Transport, Request->AckTimeoutSeconds, Request->Log,
                                      Request->UserData);
            if (Ack == HotSendAckWritten) {
                break;
            }
            Attempt++;
            if (Attempt > (int)HotSendRetryMax) {
                ExitCode = HotSendWaitVerdict(Transport, Request->VerifyTimeoutSeconds,
                                              Request->Log, Request->UserData);
                goto Done;
            }
            snprintf(Note, sizeof(Note), "block %u: retry %d/%d", (unsigned)BlockIndex,
                     Attempt, HotSendRetryMax);
            HotSendSessionLog(Request, Note);
        }
        for (Scan = 0; Scan <= Position; Scan++) {
            size_t Off = (size_t)Plan[Scan] * HotSendBlockBytes;
            uint32_t Len = (uint32_t)(FileLength - Off);

            if (Len > HotSendBlockBytes) {
                Len = HotSendBlockBytes;
            }
            SentBytes += Len;
        }
        if (Request->Progress != NULL) {
            Request->Progress(Position + 1, PlanCount, SentBytes, Request->UserData);
        }
        snprintf(Note, sizeof(Note), "sent %u/%u blocks (%u%%, %u KB)",
                 (unsigned)(Position + 1), (unsigned)PlanCount,
                 PlanCount ? (100u * (Position + 1)) / PlanCount : 100u,
                 (unsigned)(SentBytes / 1024u));
        HotSendSessionLog(Request, Note);
        if (Request->PaceMilliseconds > 0) {
            HotSendSessionSleepSeconds((double)Request->PaceMilliseconds / 1000.0);
        }
        if (Request->SteadyThrottle && ((Position + 1) % 16u) == 0u) {
            double PaceSeconds = (double)Request->PaceMilliseconds / 1000.0;
            double Gap = PaceSeconds > 0.5 ? PaceSeconds : 0.5;

            HotSendSessionSleepSeconds(Gap * 4.0);
        }
    }

    {
        unsigned char Trailer[16];

        HotSendPutUint32Little(Trailer + 0, HotSendMagic);
        HotSendPutUint32Little(Trailer + 4, HotSendTrailerIndex);
        HotSendPutUint32Little(Trailer + 8, 0);
        HotSendPutUint32Little(Trailer + 12, WholeCrc);
        snprintf(Note, sizeof(Note), "blocks done, sending trailer crc=%08x",
                 (unsigned)WholeCrc);
        HotSendSessionLog(Request, Note);
        if (Transport->WriteAll(Transport, Trailer, sizeof(Trailer),
                                Request->AckTimeoutSeconds) != 0) {
            ExitCode = HotSendExitProtocol;
            goto Done;
        }
    }
    ExitCode = HotSendWaitVerdict(Transport, Request->VerifyTimeoutSeconds, Request->Log,
                                  Request->UserData);

Done:
    free(FileData);
    free(LocalCrcs);
    free(BoardCrcs);
    free(Plan);
    return ExitCode;
}
