/*
 * Config.c — hotsend.conf（CLI / Gui 共用默认）
 */
#include "Config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void HotSendConfigClear(HotSendConfig *Config) {
    if (Config == NULL) {
        return;
    }
    memset(Config, 0, sizeof(*Config));
    Config->PreferSerial = -1;
    Config->BaudRate = -1;
    Config->TcpPort = -1;
    Config->PaceMilliseconds = -1;
}

static void HotSendConfigTrim(char *Text) {
    char *Start;
    char *End;

    if (Text == NULL) {
        return;
    }
    Start = Text;
    while (*Start != 0 && isspace((unsigned char)*Start)) {
        Start++;
    }
    if (Start != Text) {
        memmove(Text, Start, strlen(Start) + 1);
    }
    End = Text + strlen(Text);
    while (End > Text && isspace((unsigned char)End[-1])) {
        End--;
    }
    *End = 0;
}

static int HotSendConfigExpandHome(const char *Input, char *Out, size_t Capacity) {
    const char *Home;

    if (Input == NULL || Out == NULL || Capacity == 0) {
        return -1;
    }
    if (Input[0] != '~') {
        if (strlen(Input) + 1 > Capacity) {
            return -1;
        }
        memcpy(Out, Input, strlen(Input) + 1);
        return 0;
    }
    Home = getenv("HOME");
    if (Home == NULL) {
        return -1;
    }
    if (Input[1] == '/' || Input[1] == 0) {
        if (snprintf(Out, Capacity, "%s%s", Home, Input[1] == 0 ? "" : Input + 1) < 0 ||
            strlen(Out) + 1 > Capacity) {
            return -1;
        }
        return 0;
    }
    return -1;
}

static int HotSendConfigSetString(char *Out, size_t Capacity, int *HaveFlag,
                                  const char *Value) {
    char Expanded[512];

    if (HotSendConfigExpandHome(Value, Expanded, sizeof(Expanded)) != 0) {
        return -1;
    }
    if (strlen(Expanded) + 1 > Capacity) {
        return -1;
    }
    memcpy(Out, Expanded, strlen(Expanded) + 1);
    if (HaveFlag != NULL) {
        *HaveFlag = 1;
    }
    return 0;
}

static int HotSendConfigParseBool(const char *Value, int *Out) {
    if (strcmp(Value, "1") == 0 || strcmp(Value, "true") == 0 ||
        strcmp(Value, "yes") == 0 || strcmp(Value, "on") == 0) {
        *Out = 1;
        return 0;
    }
    if (strcmp(Value, "0") == 0 || strcmp(Value, "false") == 0 ||
        strcmp(Value, "no") == 0 || strcmp(Value, "off") == 0) {
        *Out = 0;
        return 0;
    }
    return -1;
}

static int HotSendConfigApplyLine(HotSendConfig *Config, char *Line) {
    char *Equals;
    char *Key;
    char *Value;
    int BoolValue;

    HotSendConfigTrim(Line);
    if (Line[0] == 0 || Line[0] == '#' || Line[0] == ';') {
        return 0;
    }
    Equals = strchr(Line, '=');
    if (Equals == NULL) {
        return -1;
    }
    *Equals = 0;
    Key = Line;
    Value = Equals + 1;
    HotSendConfigTrim(Key);
    HotSendConfigTrim(Value);
    if (strcmp(Key, "host") == 0) {
        return HotSendConfigSetString(Config->Host, sizeof(Config->Host),
                                      &Config->HaveHost, Value);
    }
    if (strcmp(Key, "port") == 0 || strcmp(Key, "device") == 0) {
        return HotSendConfigSetString(Config->SerialDevicePath,
                                      sizeof(Config->SerialDevicePath),
                                      &Config->HaveSerial, Value);
    }
    if (strcmp(Key, "file") == 0 || strcmp(Key, "elf") == 0) {
        return HotSendConfigSetString(Config->LocalFilePath,
                                      sizeof(Config->LocalFilePath), &Config->HaveFile,
                                      Value);
    }
    if (strcmp(Key, "path") == 0) {
        return HotSendConfigSetString(Config->BoardTargetPath,
                                      sizeof(Config->BoardTargetPath),
                                      &Config->HavePath, Value);
    }
    if (strcmp(Key, "baud") == 0) {
        Config->BaudRate = atoi(Value);
        return 0;
    }
    if (strcmp(Key, "tcp-port") == 0) {
        Config->TcpPort = atoi(Value);
        return 0;
    }
    if (strcmp(Key, "pace-ms") == 0) {
        Config->PaceMilliseconds = atoi(Value);
        return 0;
    }
    if (strcmp(Key, "transport") == 0) {
        if (strcmp(Value, "serial") == 0) {
            Config->PreferSerial = 1;
            return 0;
        }
        if (strcmp(Value, "tcp") == 0) {
            Config->PreferSerial = 0;
            return 0;
        }
        return -1;
    }
    if (strcmp(Key, "no-reboot") == 0) {
        return HotSendConfigParseBool(Value, &Config->NoReboot) == 0 ? 0 : -1;
    }
    if (strcmp(Key, "no-backup") == 0) {
        return HotSendConfigParseBool(Value, &Config->NoBackup) == 0 ? 0 : -1;
    }
    if (strcmp(Key, "no-prealloc") == 0) {
        return HotSendConfigParseBool(Value, &Config->NoPreallocate) == 0 ? 0 : -1;
    }
    if (strcmp(Key, "dry-run") == 0) {
        return HotSendConfigParseBool(Value, &Config->DryRun) == 0 ? 0 : -1;
    }
    if (strcmp(Key, "info") == 0) {
        return HotSendConfigParseBool(Value, &Config->InfoOnly) == 0 ? 0 : -1;
    }
    if (strcmp(Key, "steady-throttle") == 0) {
        return HotSendConfigParseBool(Value, &Config->SteadyThrottle) == 0 ? 0 : -1;
    }
    (void)BoolValue;
    return -1;
}

static int HotSendConfigLoadFile(HotSendConfig *Config, const char *Path) {
    FILE *File;
    char Line[640];
    int Any = 0;

    File = fopen(Path, "r");
    if (File == NULL) {
        return -1;
    }
    while (fgets(Line, (int)sizeof(Line), File) != NULL) {
        if (HotSendConfigApplyLine(Config, Line) == 0) {
            Any = 1;
        }
    }
    fclose(File);
    if (Any) {
        snprintf(Config->LoadedFrom, sizeof(Config->LoadedFrom), "%s", Path);
    }
    return Any ? 0 : -1;
}

int HotSendConfigLoad(HotSendConfig *Config, const char *ExplicitPath) {
    char Candidate[512];
    const char *Home;
    const char *EnvPath;

    if (Config == NULL) {
        return -1;
    }
    HotSendConfigClear(Config);
    if (ExplicitPath != NULL && ExplicitPath[0] != 0) {
        return HotSendConfigLoadFile(Config, ExplicitPath);
    }
    EnvPath = getenv("HOTSEND_CONFIG");
    if (EnvPath != NULL && EnvPath[0] != 0 &&
        HotSendConfigLoadFile(Config, EnvPath) == 0) {
        return 0;
    }
    if (HotSendConfigLoadFile(Config, "hotsend.conf") == 0) {
        return 0;
    }
    Home = getenv("HOME");
    if (Home != NULL) {
        snprintf(Candidate, sizeof(Candidate), "%s/.config/hotsend/hotsend.conf", Home);
        if (HotSendConfigLoadFile(Config, Candidate) == 0) {
            return 0;
        }
        snprintf(Candidate, sizeof(Candidate), "%s/HotSend/hotsend.conf", Home);
        if (HotSendConfigLoadFile(Config, Candidate) == 0) {
            return 0;
        }
    }
    return -1;
}

void HotSendConfigApplyToRequest(const HotSendConfig *Config,
                                 HotSendTransferRequest *Request) {
    int UseSerial;

    if (Config == NULL || Request == NULL) {
        return;
    }
    UseSerial = Config->PreferSerial;
    if (UseSerial < 0) {
        if (Config->HaveSerial && !Config->HaveHost) {
            UseSerial = 1;
        } else if (Config->HaveHost && !Config->HaveSerial) {
            UseSerial = 0;
        } else if (Config->HaveSerial) {
            UseSerial = 1;
        } else {
            UseSerial = 0;
        }
    }
    if (UseSerial) {
        if (Config->HaveSerial && Request->SerialDevicePath == NULL) {
            Request->SerialDevicePath = Config->SerialDevicePath;
        }
    } else if (Config->HaveHost && Request->Host == NULL) {
        Request->Host = Config->Host;
    }
    if (Config->HaveFile && Request->LocalFilePath == NULL) {
        Request->LocalFilePath = Config->LocalFilePath;
    }
    if (Config->HavePath) {
        Request->BoardTargetPath = Config->BoardTargetPath;
    }
    if (Config->BaudRate >= 0) {
        Request->BaudRate = Config->BaudRate;
    }
    if (Config->TcpPort > 0) {
        Request->TcpPort = Config->TcpPort;
    }
    if (Config->PaceMilliseconds >= 0) {
        Request->PaceMilliseconds = Config->PaceMilliseconds;
    }
    Request->NoReboot = Config->NoReboot;
    Request->NoBackup = Config->NoBackup;
    Request->NoPreallocate = Config->NoPreallocate;
    Request->DryRun = Config->DryRun;
    Request->InfoOnly = Config->InfoOnly;
    Request->SteadyThrottle = Config->SteadyThrottle;
}
