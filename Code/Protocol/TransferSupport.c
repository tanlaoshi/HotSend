/*
 * TransferSupport.c — 读文件 / 触发行 / 帧拼装（Transfer 辅助）
 */
#include "TransferSupport.h"

#include "Constants.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int HotSendLoadLocalFile(const char *Path, unsigned char **OutData, size_t *OutLength) {
    FILE *File;
    long Size;
    unsigned char *Data;

    File = fopen(Path, "rb");
    if (File == NULL) {
        return -1;
    }
    if (fseek(File, 0, SEEK_END) != 0) {
        fclose(File);
        return -1;
    }
    Size = ftell(File);
    if (Size < 0 || (size_t)Size > HotSendFileBytesMax || Size == 0) {
        fclose(File);
        return -1;
    }
    if (fseek(File, 0, SEEK_SET) != 0) {
        fclose(File);
        return -1;
    }
    Data = (unsigned char *)malloc((size_t)Size);
    if (Data == NULL) {
        fclose(File);
        return -1;
    }
    if (fread(Data, 1, (size_t)Size, File) != (size_t)Size) {
        free(Data);
        fclose(File);
        return -1;
    }
    fclose(File);
    *OutData = Data;
    *OutLength = (size_t)Size;
    return 0;
}

int HotSendBuildTriggerCommand(char *Out, size_t Capacity, const char *Target,
                               int NoReboot, int NoBackup) {
    int Written;

    if (strcmp(Target, HOT_SEND_DEFAULT_TARGET_PATH) == 0 && !NoReboot && !NoBackup) {
        if (Capacity < 7) {
            return -1;
        }
        memcpy(Out, "ksend\n", 7);
        return 0;
    }
    Written = snprintf(Out, Capacity, "ksend%s%s%s%s\n", NoReboot ? " --no-reboot" : "",
                       NoBackup ? " --no-backup" : "",
                       strcmp(Target, HOT_SEND_DEFAULT_TARGET_PATH) != 0 ? " " : "",
                       strcmp(Target, HOT_SEND_DEFAULT_TARGET_PATH) != 0 ? Target : "");
    if (Written < 0 || (size_t)Written >= Capacity) {
        return -1;
    }
    return 0;
}

void HotSendPutUint32Little(unsigned char *Out, uint32_t Value) {
    Out[0] = (unsigned char)(Value & 0xffu);
    Out[1] = (unsigned char)((Value >> 8) & 0xffu);
    Out[2] = (unsigned char)((Value >> 16) & 0xffu);
    Out[3] = (unsigned char)((Value >> 24) & 0xffu);
}

int HotSendSendAbortFrame(HotSendTransport *Transport) {
    unsigned char Frame[16];

    HotSendPutUint32Little(Frame + 0, HotSendMagic);
    HotSendPutUint32Little(Frame + 4, HotSendAbortIndex);
    HotSendPutUint32Little(Frame + 8, 0);
    HotSendPutUint32Little(Frame + 12, 0);
    return Transport->WriteAll(Transport, Frame, sizeof(Frame), 5.0);
}

int HotSendWaitVerdict(HotSendTransport *Transport, double TimeoutSeconds,
                       HotSendLogFunction Log, void *UserData) {
    /* 允无尾随空格：板行是 "KSEND OK crc=…" */
    static const char *const Prefixes[] = {"KSEND OK", "KSEND FAIL"};
    char Line[256];
    char Note[128];
    int Status;

    if (Log != NULL) {
        Log("waiting for KSEND OK/FAIL …", UserData);
    }
    Status = HotSendWaitForLinePrefix(Transport, Prefixes, 2, Line, sizeof(Line),
                                      TimeoutSeconds, Log, UserData);
    if (Status != 0) {
        snprintf(Note, sizeof(Note),
                 "verdict: no OK/FAIL on TCP (errno=%d); board serial may still show OK",
                 errno);
        if (Log != NULL) {
            Log(Note, UserData);
        }
        return HotSendExitProtocol;
    }
    if (Log != NULL) {
        Log(Line, UserData);
    }
    if (strncmp(Line, "KSEND FAIL", 10) == 0) {
        static const char *const RestorePrefixes[] = {"KSEND RESTORED",
                                                      "KSEND RESTORE FAIL"};
        char RestoreLine[256];

        RestoreLine[0] = 0;
        (void)HotSendWaitForLinePrefix(Transport, RestorePrefixes, 2, RestoreLine,
                                       sizeof(RestoreLine), 180.0, Log, UserData);
        if (RestoreLine[0] != 0 && Log != NULL) {
            Log(RestoreLine, UserData);
        }
        return HotSendExitBoardFail;
    }
    return HotSendExitOk;
}
