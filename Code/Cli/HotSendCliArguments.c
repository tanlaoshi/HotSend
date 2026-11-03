/*
 * HotSendCliArguments.c — 命令行参数（对表 Tools/ksend.py 常用项）
 */
#include "HotSendCliArguments.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void HotSendCliPrintUsage(const char *ProgramName) {
    fprintf(stderr,
            "usage: %s (--host <ipv4> | --port <device>) --file <path> [options]\n"
            "\n"
            "  --host <ipv4>       TCP (board: ksend serve)\n"
            "  --port <device>      serial e.g. /dev/ttyUSB0 (exclusive with --host)\n"
            "  --baud <n>          serial baud (default 115200)\n"
            "  --file|--elf <path> local file (required)\n"
            "  --path <board>      board target (default BLOCKS:Kernel.elf)\n"
            "  --tcp-port <n>      TCP port (default 26992)\n"
            "  --no-reboot         leave board shell after OK\n"
            "  --no-backup         skip board .bak\n"
            "  --ready-timeout <s> --ack-timeout <s>\n"
            "  --data-timeout <s>  --verify-timeout <s>\n"
            "  --help\n",
            ProgramName != NULL ? ProgramName : "hotsend");
}

void HotSendCliArgumentsClear(HotSendCliArguments *Arguments) {
    if (Arguments == NULL) {
        return;
    }
    memset(Arguments, 0, sizeof(*Arguments));
    HotSendTransferRequestInit(&Arguments->Request);
}

static int HotSendCliTakeString(int *Index, int ArgumentCount, char **ArgumentVector,
                                const char **Out) {
    if (*Index + 1 >= ArgumentCount) {
        return -1;
    }
    *Out = ArgumentVector[++(*Index)];
    return 0;
}

static int HotSendCliTakeDouble(int *Index, int ArgumentCount, char **ArgumentVector,
                                double *Out) {
    const char *Text;

    if (HotSendCliTakeString(Index, ArgumentCount, ArgumentVector, &Text) != 0) {
        return -1;
    }
    *Out = atof(Text);
    return 0;
}

static int HotSendCliTakeInt(int *Index, int ArgumentCount, char **ArgumentVector,
                             int *Out) {
    const char *Text;

    if (HotSendCliTakeString(Index, ArgumentCount, ArgumentVector, &Text) != 0) {
        return -1;
    }
    *Out = atoi(Text);
    return 0;
}

int HotSendCliArgumentsParse(HotSendCliArguments *Arguments, int ArgumentCount,
                             char **ArgumentVector) {
    int Index;

    if (Arguments == NULL) {
        return -1;
    }
    HotSendCliArgumentsClear(Arguments);
    for (Index = 1; Index < ArgumentCount; Index++) {
        const char *Token = ArgumentVector[Index];

        if (strcmp(Token, "--help") == 0 || strcmp(Token, "-h") == 0) {
            Arguments->ShowHelp = 1;
            return 0;
        }
        if (strcmp(Token, "--host") == 0) {
            if (HotSendCliTakeString(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.Host) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--port") == 0) {
            if (HotSendCliTakeString(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.SerialDevicePath) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--baud") == 0) {
            if (HotSendCliTakeInt(&Index, ArgumentCount, ArgumentVector,
                                  &Arguments->Request.BaudRate) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--file") == 0 || strcmp(Token, "--elf") == 0) {
            if (HotSendCliTakeString(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.LocalFilePath) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--path") == 0) {
            if (HotSendCliTakeString(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.BoardTargetPath) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--tcp-port") == 0) {
            if (HotSendCliTakeInt(&Index, ArgumentCount, ArgumentVector,
                                  &Arguments->Request.TcpPort) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--no-reboot") == 0) {
            Arguments->Request.NoReboot = 1;
        } else if (strcmp(Token, "--no-backup") == 0) {
            Arguments->Request.NoBackup = 1;
        } else if (strcmp(Token, "--ready-timeout") == 0) {
            if (HotSendCliTakeDouble(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.ReadyTimeoutSeconds) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--ack-timeout") == 0) {
            if (HotSendCliTakeDouble(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.AckTimeoutSeconds) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--data-timeout") == 0) {
            if (HotSendCliTakeDouble(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.DataTimeoutSeconds) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else if (strcmp(Token, "--verify-timeout") == 0) {
            if (HotSendCliTakeDouble(&Index, ArgumentCount, ArgumentVector,
                                     &Arguments->Request.VerifyTimeoutSeconds) != 0) {
                Arguments->ParseError = 1;
                return -1;
            }
        } else {
            fprintf(stderr, "unknown argument: %s\n", Token);
            Arguments->ParseError = 1;
            return -1;
        }
    }
    if (Arguments->Request.LocalFilePath == NULL) {
        Arguments->ParseError = 1;
        return -1;
    }
    if ((Arguments->Request.Host == NULL) ==
        (Arguments->Request.SerialDevicePath == NULL)) {
        /* 必须恰好一个：Host 或 Serial */
        Arguments->ParseError = 1;
        return -1;
    }
    return 0;
}
