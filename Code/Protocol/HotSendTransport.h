#ifndef HOT_SEND_TRANSPORT_H
#define HOT_SEND_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

typedef struct HotSendTransport HotSendTransport;

struct HotSendTransport {
    void *State;
    int (*WriteAll)(HotSendTransport *Self, const void *Data, size_t Length,
                    double TimeoutSeconds);
    int (*ReadExact)(HotSendTransport *Self, void *Data, size_t Length,
                     double TimeoutSeconds);
    int (*ReadLine)(HotSendTransport *Self, char *Line, size_t Capacity,
                    double TimeoutSeconds);
    void (*Close)(HotSendTransport *Self);
};

#endif
