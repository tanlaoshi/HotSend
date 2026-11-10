#ifndef HOT_SEND_GUI_WIDGETS_H
#define HOT_SEND_GUI_WIDGETS_H

#include <gtk/gtk.h>

typedef struct HotSendGuiWidgets {
    GtkApplication *Application;
    GtkWindow *Window;
    GtkLabel *BrandTitleLabel;
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
    GtkTextMark *LogProgressMark; /* 进度/等待同行刷新；Append 时清掉 */
    const char *BrowseDialogTitle;
    int Language;
    GThread *WorkerThread;
    volatile int CancelRequested;
    volatile int TransferRunning;
} HotSendGuiWidgets;

GtkWidget *HotSendGuiBuildWindow(HotSendGuiWidgets *Widgets);
void HotSendGuiAppendLog(HotSendGuiWidgets *Widgets, const char *Line);
/* 覆盖上一条进度行（无则新开一行）；用于 sent / waiting … */
void HotSendGuiRefreshLog(HotSendGuiWidgets *Widgets, const char *Line);
void HotSendGuiClearLog(HotSendGuiWidgets *Widgets);
int HotSendLogLineIsProgress(const char *Line);

#endif
