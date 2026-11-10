/*
 * Main.c — 正式 CLI 入口（PR-2）
 */
#include "Arguments.h"
#include "Protocol.h"

#include <stdio.h>
#include <string.h>

static int gCliProgressLineActive;

static int HotSendCliLogIsProgress(const char *Message) {
    if (Message == NULL) {
        return 0;
    }
    if (strncmp(Message, "sent ", 5) == 0) {
        return 1;
    }
    if (strncmp(Message, "waiting for board line", 22) == 0) {
        return 1;
    }
    if (strncmp(Message, "waiting for KSEND", 17) == 0) {
        return 1;
    }
    if (strncmp(Message, "progress ", 9) == 0) {
        return 1;
    }
    return 0;
}

static void HotSendCliEndProgressLine(void) {
    if (gCliProgressLineActive) {
        fputc('\n', stderr);
        gCliProgressLineActive = 0;
    }
}

static void HotSendCliLog(const char *Message, void *UserData) {
    (void)UserData;
    if (HotSendCliLogIsProgress(Message)) {
        fprintf(stderr, "\r\033[K%s", Message != NULL ? Message : "");
        fflush(stderr);
        gCliProgressLineActive = 1;
        return;
    }
    HotSendCliEndProgressLine();
    fprintf(stderr, "%s\n", Message != NULL ? Message : "");
    fflush(stderr);
}

static void HotSendCliProgress(uint32_t Position, uint32_t Total, uint32_t SentBytes,
                               void *UserData) {
    char Note[96];

    (void)UserData;
    snprintf(Note, sizeof(Note), "progress %u/%u (%u KB)", (unsigned)Position,
             (unsigned)Total, (unsigned)(SentBytes / 1024u));
    HotSendCliLog(Note, NULL);
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

    if (Arguments.ConfigLoaded) {
        fprintf(stderr, "config: %s\n", Arguments.Config.LoadedFrom);
    }
    Arguments.Request.Log = HotSendCliLog;
    Arguments.Request.Progress = HotSendCliProgress;
    if (Arguments.Request.AckTimeoutSeconds < 120.0) {
        Arguments.Request.AckTimeoutSeconds = 120.0;
    }
    if (Arguments.Request.DataTimeoutSeconds < 180.0) {
        Arguments.Request.DataTimeoutSeconds = 180.0;
    }

    if (Arguments.Request.SerialDevicePath != NULL) {
        ExitCode = HotSendTransferOverSerial(&Arguments.Request);
    } else {
        ExitCode = HotSendTransferOverTcp(&Arguments.Request);
    }
    HotSendCliEndProgressLine();
    if (ExitCode == HotSendExitOk) {
        fprintf(stderr, "hotsend: OK\n");
    } else {
        fprintf(stderr, "hotsend: exit %d\n", ExitCode);
    }
    return ExitCode;
}
