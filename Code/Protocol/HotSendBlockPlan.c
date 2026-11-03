/*
 * HotSendBlockPlan.c — 路径校验 / 块 CRC / 差量计划
 */
#include "HotSendBlockPlan.h"

#include "HotSendConstants.h"
#include "HotSendCrc32.h"

#include <string.h>

int HotSendValidateBoardTargetPath(const char *Path) {
    size_t Length;
    size_t Index;
    const char *Name;
    const char *Slash;
    const char *Colon;

    if (Path == NULL || Path[0] == 0) {
        return -1;
    }
    Length = strlen(Path);
    if (Length > HotSendTargetPathMax) {
        return -1;
    }
    for (Index = 0; Index < Length; Index++) {
        unsigned char Byte = (unsigned char)Path[Index];

        if (Byte < HotSendPathCharMin || Byte > HotSendPathCharMax) {
            return -1;
        }
    }
    if (Length + strlen(HOT_SEND_BACKUP_SUFFIX) > HotSendBackupPathMax) {
        return -1;
    }
    Colon = strrchr(Path, ':');
    Slash = strrchr(Path, '/');
    Name = Path;
    if (Colon != NULL && Colon + 1 > Name) {
        Name = Colon + 1;
    }
    if (Slash != NULL && Slash + 1 > Name) {
        Name = Slash + 1;
    }
    if (Name[0] == 0) {
        return -1;
    }
    return 0;
}

int HotSendComputeBlockCrcs(const unsigned char *Data, size_t Length,
                            uint32_t *OutCrcs, uint32_t *OutCount) {
    uint32_t Count;
    uint32_t Index;

    if (Data == NULL || OutCrcs == NULL || OutCount == NULL) {
        return -1;
    }
    if (Length == 0 || Length > HotSendFileBytesMax) {
        return -1;
    }
    Count = (uint32_t)((Length + HotSendBlockBytes - 1) / HotSendBlockBytes);
    for (Index = 0; Index < Count; Index++) {
        size_t Offset = (size_t)Index * HotSendBlockBytes;
        size_t Take = Length - Offset;

        if (Take > HotSendBlockBytes) {
            Take = HotSendBlockBytes;
        }
        OutCrcs[Index] = HotSendCrc32Buffer(Data + Offset, Take);
    }
    *OutCount = Count;
    return 0;
}

uint32_t HotSendComputeWholeCrc(const unsigned char *Data, size_t Length) {
    return HotSendCrc32Buffer(Data, Length);
}

uint32_t HotSendBuildDiffPlan(const uint32_t *LocalCrcs, uint32_t LocalCount,
                              const uint32_t *BoardCrcs, uint32_t BoardCount,
                              uint32_t *OutPlan) {
    uint32_t PlanCount = 0;
    uint32_t Index;
    uint32_t Last;
    int LastPresent = 0;

    if (LocalCrcs == NULL || OutPlan == NULL || LocalCount == 0) {
        return 0;
    }
    for (Index = 0; Index < LocalCount; Index++) {
        if (Index >= BoardCount || LocalCrcs[Index] != BoardCrcs[Index]) {
            OutPlan[PlanCount++] = Index;
            if (Index + 1 == LocalCount) {
                LastPresent = 1;
            }
        }
    }
    Last = LocalCount - 1;
    if (!LastPresent) {
        OutPlan[PlanCount++] = Last;
    }
    return PlanCount;
}
