#ifndef HOT_SEND_CRC32_H
#define HOT_SEND_CRC32_H

#include <stddef.h>
#include <stdint.h>

/* 与 Python zlib.crc32 / 板 KsendCrc32Final(Update(INITIAL,…)) 同参 */
uint32_t HotSendCrc32Buffer(const void *Data, size_t Length);

#endif
