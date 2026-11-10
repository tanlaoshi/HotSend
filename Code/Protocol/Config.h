#ifndef HOT_SEND_CONFIG_H
#define HOT_SEND_CONFIG_H

#include "Protocol.h"

/*
 * 默认配置：key=value 文本。查找顺序见 Config.c。
 * 缓冲由本结构持有；Apply 后 Request 里的指针指向这些缓冲。
 */
typedef struct HotSendConfig {
    char Host[64];
    char SerialDevicePath[128];
    char LocalFilePath[512];
    char BoardTargetPath[96];
    char LoadedFrom[512];
    int HaveHost;
    int HaveSerial;
    int HaveFile;
    int HavePath;
    int BaudRate;
    int TcpPort;
    int NoReboot;
    int NoBackup;
    int NoPreallocate;
    int DryRun;
    int InfoOnly;
    int PaceMilliseconds;
    int SteadyThrottle;
    int PreferSerial; /* 1=串口优先；0=TCP；-1=未写 transport= */
} HotSendConfig;

void HotSendConfigClear(HotSendConfig *Config);

/* ExplicitPath 非 NULL 则只读该文件；否则按查找顺序。0=读到至少一个键或文件存在。 */
int HotSendConfigLoad(HotSendConfig *Config, const char *ExplicitPath);

/* 把已解析的配置填进 Request（不覆盖 Request 里已非 NULL / 已显式的字段由调用方先处理）。 */
void HotSendConfigApplyToRequest(const HotSendConfig *Config,
                                 HotSendTransferRequest *Request);

#endif
