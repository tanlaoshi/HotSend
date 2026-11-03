/*
 * SmokeTcp.c — PR-1 主机自测钩子（薄 argv；完整 CLI 属 PR-2）
 *
 * 用法：
 *   hotsend-smoke-tcp --host 192.168.31.41 --file /path/Kernel.elf \
 *     [--path D:Kernel.elf] [--tcp-port 26992] [--no-reboot]
 */
#include "Protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void HotSendSmokeLog(const char *Message, void *UserData) {
    (void)UserData;
    fprintf(stderr, "%s\n", Message);
    fflush(stderr);
}

static void HotSendSmokeUsage(const char *Argv0) {
    fprintf(stderr,
            "usage: %s --host <ipv4> --file <path> [--path <board>] "
            "[--tcp-port N] [--no-reboot] [--no-backup]\n",
            Argv0);
}

int main(int Argc, char **Argv) {
    HotSendTransferRequest Request;
    int Index;
    int ExitCode;

    HotSendTransferRequestInit(&Request);
    Request.Log = HotSendSmokeLog;
    Request.NoReboot = 1;

    for (Index = 1; Index < Argc; Index++) {
        if (strcmp(Argv[Index], "--host") == 0 && Index + 1 < Argc) {
            Request.Host = Argv[++Index];
        } else if (strcmp(Argv[Index], "--file") == 0 && Index + 1 < Argc) {
            Request.LocalFilePath = Argv[++Index];
        } else if (strcmp(Argv[Index], "--path") == 0 && Index + 1 < Argc) {
            Request.BoardTargetPath = Argv[++Index];
        } else if (strcmp(Argv[Index], "--tcp-port") == 0 && Index + 1 < Argc) {
            Request.TcpPort = atoi(Argv[++Index]);
        } else if (strcmp(Argv[Index], "--no-reboot") == 0) {
            Request.NoReboot = 1;
        } else if (strcmp(Argv[Index], "--no-backup") == 0) {
            Request.NoBackup = 1;
        } else if (strcmp(Argv[Index], "--help") == 0) {
            HotSendSmokeUsage(Argv[0]);
            return HotSendExitLocal;
        } else {
            fprintf(stderr, "unknown arg: %s\n", Argv[Index]);
            HotSendSmokeUsage(Argv[0]);
            return HotSendExitLocal;
        }
    }
    if (Request.Host == NULL || Request.LocalFilePath == NULL) {
        HotSendSmokeUsage(Argv[0]);
        return HotSendExitLocal;
    }
    Request.AckTimeoutSeconds = 120.0;
    Request.DataTimeoutSeconds = 180.0;
    ExitCode = HotSendTransferOverTcp(&Request);
    if (ExitCode == HotSendExitOk) {
        fprintf(stderr, "hotsend-smoke-tcp: OK\n");
    } else {
        fprintf(stderr, "hotsend-smoke-tcp: exit %d\n", ExitCode);
    }
    return ExitCode;
}
