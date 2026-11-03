#ifndef HOT_SEND_GUI_LAYOUT_SUPPORT_H
#define HOT_SEND_GUI_LAYOUT_SUPPORT_H

#include "Widgets.h"

void HotSendGuiShowTransportPage(HotSendGuiWidgets *Widgets, int Serial);
void HotSendGuiOnTransportToggled(GtkCheckButton *Button, gpointer UserData);
void HotSendGuiOnLanguageToggled(GtkCheckButton *Button, gpointer UserData);
void HotSendGuiOnRefreshDevices(GtkButton *Button, gpointer UserData);
void HotSendGuiApplyConfig(HotSendGuiWidgets *Widgets);
void HotSendGuiOnBrowseFile(GtkButton *Button, gpointer UserData);
void HotSendGuiOnStart(GtkButton *Button, gpointer UserData);
void HotSendGuiOnAbort(GtkButton *Button, gpointer UserData);

#endif
