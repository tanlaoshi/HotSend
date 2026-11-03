#ifndef HOT_SEND_CLI_ARGUMENTS_H
#define HOT_SEND_CLI_ARGUMENTS_H

#include "Config.h"
#include "Protocol.h"

typedef struct HotSendCliArguments {
    HotSendTransferRequest Request;
    HotSendConfig Config; /* Request 字符串指针可指向此处缓冲 */
    int ShowHelp;
    int ParseError;
    int ConfigLoaded;
} HotSendCliArguments;

void HotSendCliArgumentsClear(HotSendCliArguments *Arguments);
int HotSendCliArgumentsParse(HotSendCliArguments *Arguments, int ArgumentCount,
                             char **ArgumentVector);
void HotSendCliPrintUsage(const char *ProgramName);

#endif
