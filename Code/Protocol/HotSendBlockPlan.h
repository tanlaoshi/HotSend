#ifndef HOT_SEND_BLOCK_PLAN_H
#define HOT_SEND_BLOCK_PLAN_H

#include <stddef.h>
#include <stdint.h>

int HotSendValidateBoardTargetPath(const char *Path);

/* 为本地缓冲算每块 CRC；OutCount = 块数。调用方提供 OutCrcs[块数] */
int HotSendComputeBlockCrcs(const unsigned char *Data, size_t Length,
                            uint32_t *OutCrcs, uint32_t *OutCount);

uint32_t HotSendComputeWholeCrc(const unsigned char *Data, size_t Length);

/*
 * 差量下标写入 OutPlan，返回块数。最后一块恒包含（推尺寸）。
 * OutPlan 容量须 >= LocalCount。
 */
uint32_t HotSendBuildDiffPlan(const uint32_t *LocalCrcs, uint32_t LocalCount,
                              const uint32_t *BoardCrcs, uint32_t BoardCount,
                              uint32_t *OutPlan);

#endif
