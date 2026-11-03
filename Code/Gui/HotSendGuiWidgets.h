#ifndef HOT_SEND_GUI_WIDGETS_H
#define HOT_SEND_GUI_WIDGETS_H

#include <gtk/gtk.h>

typedef struct HotSendGuiWidgets {
    GtkApplication *Application;
    GtkWindow *Window;
    GtkEntry *HostEntry;
    GtkEntry *PortEntry;
    GtkEntry *FileEntry;
    GtkDropDown *BoardPathDropDown;
    GtkCheckButton *UseSerialCheck;
    GtkCheckButton *NoRebootCheck;
    GtkCheckButton *NoBackupCheck;
    GtkButton *StartButton;
    GtkButton *AbortButton;
    GtkProgressBar *ProgressBar;
    GtkLabel *StatusLabel;
    GtkTextBuffer *LogBuffer;
    GtkTextView *LogView;
    GThread *WorkerThread;
    volatile int CancelRequested;
    volatile int TransferRunning;
} HotSendGuiWidgets;

GtkWidget *HotSendGuiBuildWindow(HotSendGuiWidgets *Widgets);
void HotSendGuiAppendLog(HotSendGuiWidgets *Widgets, const char *Line);

#endif

