/*
 * Constants.h — 与板端 Ksend / Tools/ksend.py 对齐的协议常量
 */
#ifndef HOT_SEND_CONSTANTS_H
#define HOT_SEND_CONSTANTS_H

enum {
    HotSendBlockBytes = 4096,
    HotSendMagic = 0x4B53424Bu,
    HotSendTrailerIndex = 0xFFFFFFFFu,
    HotSendAbortIndex = 0xFFFFFFFEu,
    HotSendAckWritten = 0x41, /* 'A' */
    HotSendAckRetry = 0x4E,   /* 'N' */
    HotSendDefaultTcpPort = 26992,
    HotSendBoardPathBytes = 96,
    HotSendBackupPathMax = 95,
    HotSendTargetPathMax = 91, /* 95 - strlen(".bak") */
    HotSendFileBytesMax = 8 * 1024 * 1024,
    HotSendMaxBlocks = HotSendFileBytesMax / HotSendBlockBytes,
    HotSendRetryMax = 8,
    HotSendPathCharMin = 0x21,
    HotSendPathCharMax = 0x7E,
    HotSendTcpMssDefault = 1200
};

#define HOT_SEND_DEFAULT_TARGET_PATH "BLOCKS:Kernel.elf"
#define HOT_SEND_BACKUP_SUFFIX ".bak"

enum HotSendExitCode {
    HotSendExitOk = 0,
    HotSendExitBoardFail = 1,
    HotSendExitProtocol = 2,
    HotSendExitLocal = 3
};

/* READY via=（nuc-2）；缺省=Unknown（旧板） */
enum HotSendReadyVia {
    HotSendViaUnknown = 0,
    HotSendViaWired = 1,
    HotSendViaWifi = 2,
    HotSendViaSerial = 3
};

#endif
