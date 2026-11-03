#ifndef HOT_SEND_GUI_SERIAL_PORTS_H
#define HOT_SEND_GUI_SERIAL_PORTS_H

#include "Widgets.h"

void HotSendGuiSerialRefreshDevices(HotSendGuiWidgets *Widgets,
                                    const char *PreferDevice);
void HotSendGuiSerialSelectBaud(HotSendGuiWidgets *Widgets, int BaudRate);
GtkStringList *HotSendGuiSerialBaudListNew(void);
const char *HotSendGuiDropDownSelectedText(GtkDropDown *DropDown);

#endif
