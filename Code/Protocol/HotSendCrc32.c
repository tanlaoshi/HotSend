/*
 * HotSendCrc32.c — 主机侧 CRC32（zlib，对表 ksend.py）
 */
#include "HotSendCrc32.h"

#include <zlib.h>

uint32_t HotSendCrc32Buffer(const void *Data, size_t Length) {
    uLong Value;

    Value = crc32(0L, Z_NULL, 0);
    Value = crc32(Value, (const Bytef *)Data, (uInt)Length);
    return (uint32_t)(Value & 0xffffffffu);
}
