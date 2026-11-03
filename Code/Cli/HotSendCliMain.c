/*
 * HotSendCliMain.c — 正式 CLI 入口（PR-2）
 */
#include "HotSendCliArguments.h"
#include "HotSendProtocol.h"

#include <stdio.h>

static void HotSendCliLog(const char *Message, void *UserData) {
    (void)UserData;
    fprintf(stderr, "%s\n", Message);
    fflush(stderr);
}

static void HotSendCliProgress(uint32_t Position, uint32_t Total, uint32_t SentBytes,
                               void *UserData) {
    (void)UserData;
    fprintf(stderr, "progress %u/%u (%u KB)\n", (unsigned)Position, (unsigned)Total,
            (unsigned)(SentBytes / 1024u));
    fflush(stderr);
}

int main(int ArgumentCount, char **ArgumentVector) {
    HotSendCliArguments Arguments;
    int ExitCode;

    if (HotSendCliArgumentsParse(&Arguments, ArgumentCount, ArgumentVector) != 0) {
        HotSendCliPrintUsage(ArgumentVector[0]);
        return HotSendExitLocal;
    }
    if (Arguments.ShowHelp) {
        HotSendCliPrintUsage(ArgumentVector[0]);
        return HotSendExitOk;
    }

    Arguments.Request.Log = HotSendCliLog;
    Arguments.Request.Progress = HotSendCliProgress;
    if (Arguments.Request.AckTimeoutSeconds < 120.0) {
        Arguments.Request.AckTimeoutSeconds = 120.0;
    }
    if (Arguments.Request.DataTimeoutSeconds < 180.0) {
        Arguments.Request.DataTimeoutSeconds = 180.0;
    }

    ExitCode = HotSendTransferOverTcp(&Arguments.Request);
    if (ExitCode == HotSendExitOk) {
        fprintf(stderr, "hotsend: OK\n");
    } else {
        fprintf(stderr, "hotsend: exit %d\n", ExitCode);
    }
    return ExitCode;
}
