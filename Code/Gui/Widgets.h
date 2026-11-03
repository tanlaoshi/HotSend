#ifndef HOT_SEND_GUI_WIDGETS_H
#define HOT_SEND_GUI_WIDGETS_H

#include <gtk/gtk.h>

typedef struct HotSendGuiWidgets {
    GtkApplication *Application;
    GtkWindow *Window;
    GtkCheckButton *EnglishRadio;
    GtkCheckButton *ChineseRadio;
    GtkCheckButton *SerialRadio;
    GtkCheckButton *TcpRadio;
    GtkStack *TransportStack;
    GtkWidget *SerialPage;
    GtkWidget *TcpPage;
    GtkLabel *DeviceLabel;
    GtkDropDown *DeviceDropDown;
    GtkButton *RefreshDevicesButton;
    GtkLabel *BaudLabel;
    GtkDropDown *BaudDropDown;
    GtkLabel *HostLabel;
    GtkEntry *HostEntry;
    GtkLabel *TcpPortLabel;
    GtkEntry *TcpPortEntry;
    GtkLabel *FileLabel;
    GtkEntry *FileEntry;
    GtkButton *BrowseButton;
    GtkLabel *BoardPathLabel;
    GtkDropDown *BoardPathDropDown;
    GtkCheckButton *NoRebootCheck;
    GtkCheckButton *NoBackupCheck;
    GtkCheckButton *DryRunCheck;
    GtkCheckButton *SteadyThrottleCheck;
    GtkButton *StartButton;
    GtkButton *AbortButton;
    GtkProgressBar *ProgressBar;
    GtkLabel *StatusLabel;
    GtkFrame *LogFrame;
    GtkTextBuffer *LogBuffer;
    GtkTextView *LogView;
    const char *BrowseDialogTitle;
    int Language;
    GThread *WorkerThread;
    volatile int CancelRequested;
    volatile int TransferRunning;
} HotSendGuiWidgets;

GtkWidget *HotSendGuiBuildWindow(HotSendGuiWidgets *Widgets);
void HotSendGuiAppendLog(HotSendGuiWidgets *Widgets, const char *Line);

#endif
