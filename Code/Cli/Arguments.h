#ifndef HOT_SEND_CLI_ARGUMENTS_H
#define HOT_SEND_CLI_ARGUMENTS_H

#include "Protocol.h"

typedef struct HotSendCliArguments {
    HotSendTransferRequest Request;
    int ShowHelp;
    int ParseError;
} HotSendCliArguments;

void HotSendCliArgumentsClear(HotSendCliArguments *Arguments);
int HotSendCliArgumentsParse(HotSendCliArguments *Arguments, int ArgumentCount,
                             char **ArgumentVector);
void HotSendCliPrintUsage(const char *ProgramName);

#endif
