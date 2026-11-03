/*
 * HotSendSerialPrompt.c — 等壳提示符 / 分块发命令（对表 ksend.py）
 *
 * 板串口提示为「Blocks> 」（见 Console HalConsoleWriteSerial）。
 * 须已退出 ksend serve / 其它占用串口的终端。
 */
#include "HotSendSerialTransport.h"

#include <errno.h>
#include <string.h>
#include <time.h>

static double HotSendSerialPromptNow(void) {
    struct timespec TimeSpec;

    clock_gettime(CLOCK_MONOTONIC, &TimeSpec);
    return (double)TimeSpec.tv_sec + (double)TimeSpec.tv_nsec / 1e9;
}

static int HotSendSerialTailHasPrompt(const unsigned char *Data, size_t Length) {
    size_t Index;

    for (Index = 0; Index < Length; Index++) {
        if (Data[Index] != (unsigned char)'>') {
            continue;
        }
        if (Index + 1 < Length && Data[Index + 1] == (unsigned char)' ') {
            return 1;
        }
        /* 宽松：单独 '>' 也认（防无空格变体） */
        if (Index + 1 >= Length || Data[Index + 1] == (unsigned char)'\r' ||
            Data[Index + 1] == (unsigned char)'\n') {
            return 1;
        }
    }
    return 0;
}

int HotSendSerialWaitShellPrompt(HotSendTransport *Transport, double TimeoutSeconds) {
    double Deadline;
    unsigned char Nudge[2] = {(unsigned char)'\r', (unsigned char)'\n'};
    unsigned char Window[256];
    size_t WindowLength = 0;
    int NudgeRound;

    if (Transport == NULL) {
        return -1;
    }
    Deadline = HotSendSerialPromptNow() + TimeoutSeconds;
    for (NudgeRound = 0; NudgeRound < 3; NudgeRound++) {
        if (Transport->WriteAll(Transport, Nudge, sizeof(Nudge), 2.0) != 0) {
            return -1;
        }
        while (HotSendSerialPromptNow() < Deadline) {
            unsigned char Byte;
            double Remain = Deadline - HotSendSerialPromptNow();

            if (Remain <= 0.0) {
                break;
            }
            if (Transport->ReadExact(Transport, &Byte, 1,
                                     Remain < 0.4 ? Remain : 0.4) != 0) {
                if (errno == ETIMEDOUT) {
                    break; /* 本轮无更多字节，再 nudge */
                }
                return -1;
            }
            if (WindowLength + 1 >= sizeof(Window)) {
                memmove(Window, Window + 1, sizeof(Window) - 1);
                WindowLength = sizeof(Window) - 1;
            }
            Window[WindowLength++] = Byte;
            if (HotSendSerialTailHasPrompt(Window, WindowLength)) {
                return 0;
            }
        }
    }
    errno = ETIMEDOUT;
    return -1;
}

int HotSendSerialSendCommandChunked(HotSendTransport *Transport, const char *Command) {
    size_t Length;
    size_t Offset;
    struct timespec Gap;

    if (Transport == NULL || Command == NULL) {
        return -1;
    }
    Length = strlen(Command);
    Gap.tv_sec = 0;
    Gap.tv_nsec = 4 * 1000 * 1000;
    for (Offset = 0; Offset < Length; Offset += 8) {
        size_t Take = Length - Offset;

        if (Take > 8) {
            Take = 8;
        }
        if (Transport->WriteAll(Transport, Command + Offset, Take, 2.0) != 0) {
            return -1;
        }
        if (Offset + Take < Length) {
            nanosleep(&Gap, NULL);
        }
    }
    if (Length == 0 || Command[Length - 1] != '\n') {
        unsigned char Newline = (unsigned char)'\n';

        if (Transport->WriteAll(Transport, &Newline, 1, 2.0) != 0) {
            return -1;
        }
    }
    return 0;
}
