#ifndef HOT_SEND_TRANSFER_SUPPORT_H
#define HOT_SEND_TRANSFER_SUPPORT_H

#include "HotSendSessionLines.h"
#include "HotSendTransport.h"

#include <stddef.h>
#include <stdint.h>

int HotSendLoadLocalFile(const char *Path, unsigned char **OutData, size_t *OutLength);
int HotSendBuildTriggerCommand(char *Out, size_t Capacity, const char *Target,
                               int NoReboot, int NoBackup);
void HotSendPutUint32Little(unsigned char *Out, uint32_t Value);
int HotSendSendAbortFrame(HotSendTransport *Transport);
int HotSendWaitVerdict(HotSendTransport *Transport, double TimeoutSeconds,
                       HotSendLogFunction Log, void *UserData);

#endif
