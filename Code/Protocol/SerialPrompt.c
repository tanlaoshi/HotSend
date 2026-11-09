/*
 * SerialPrompt.c — 等壳提示符 / 分块发命令（对表 ksend.py）
 *
 * 板串口提示为「Blocks> 」（见 Console HalConsoleWriteSerial）。
 * 须已退出 ksend serve / 其它占用串口的终端。
 *
 * nudge 只发 \\r（ksend.py 同；\\r\\n 会被板端当两次 Enter → 双 Blocks>）。
 * 冷起：先听 Announce 的 Blocks>，勿抢先敲回车——否则 FIFO 里的 \\r
 * 会在 ready 后再空 Enter，开机串口出现两次提示符。
 */
#include "SerialTransport.h"

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
    double LastRx;
    double NextNudge;
    unsigned char Nudge = (unsigned char)'\r';
    unsigned char Window[256];
    size_t WindowLength = 0;

    if (Transport == NULL) {
        return -1;
    }
    /*
     * 冷起：只读、勿狂打 CR（Pi 进桌面后协作调度会被串口洪水饿死鼠，
     * 2026-11-08）。见过输出后静默 ≥1.2s 再 nudge；整段 Timeout 内等 `>`。
     */
    if (TimeoutSeconds < 5.0) {
        TimeoutSeconds = 5.0;
    }
    Deadline = HotSendSerialPromptNow() + TimeoutSeconds;
    LastRx = HotSendSerialPromptNow();
    /*
     * 开串口常 tcflush 掉已有 Blocks>：未见 RX 时 1.2s 后发一次 \\r 唤醒。
     * 冷起有 boot 字：听 Announce 的提示符，勿先 nudge（防双 Blocks>）。
     */
    NextNudge = HotSendSerialPromptNow() + 1.2;
    while (HotSendSerialPromptNow() < Deadline) {
        unsigned char Byte;
        double Now = HotSendSerialPromptNow();
        double Remain = Deadline - Now;

        if (Now >= NextNudge) {
            if (Transport->WriteAll(Transport, &Nudge, 1, 2.0) != 0) {
                return -1;
            }
            /* 有 RX 后更稀；未见 RX（已 flush 的 idle 壳）100ms 一敲 */
            NextNudge = Now + 0.1;
        }
        if (Transport->ReadExact(Transport, &Byte, 1,
                                 Remain < 0.4 ? Remain : 0.4) != 0) {
            if (errno == ETIMEDOUT) {
                if ((Now - LastRx) >= 1.2 && NextNudge > Now) {
                    NextNudge = Now; /* 静默够久 → 下轮允许 nudge */
                }
                continue;
            }
            return -1;
        }
        LastRx = Now;
        NextNudge = Now + 1.2; /* 有输出时推迟 nudge */
        if (WindowLength + 1 >= sizeof(Window)) {
            memmove(Window, Window + 1, sizeof(Window) - 1);
            WindowLength = sizeof(Window) - 1;
        }
        Window[WindowLength++] = Byte;
        if (HotSendSerialTailHasPrompt(Window, WindowLength)) {
            return 0;
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
