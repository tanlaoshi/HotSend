#ifndef HOT_SEND_GUI_WORKER_H
#define HOT_SEND_GUI_WORKER_H

#include <gtk/gtk.h>

typedef struct HotSendGuiWidgets HotSendGuiWidgets;

void HotSendGuiWorkerStart(HotSendGuiWidgets *Widgets);
void HotSendGuiWorkerRequestCancel(HotSendGuiWidgets *Widgets);

#endif
