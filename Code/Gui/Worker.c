/*
 * Worker.c — 后台线程跑 Protocol；idle 回主线程刷 UI
 */
#include "Worker.h"

#include "Widgets.h"
#include "Protocol.h"
#include "SerialPorts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HotSendGuiIdleLog {
    HotSendGuiWidgets *Widgets;
    char *Message;
    int Refresh;
} HotSendGuiIdleLog;

typedef struct HotSendGuiIdleProgress {
    HotSendGuiWidgets *Widgets;
    uint32_t Position;
    uint32_t Total;
    uint32_t SentBytes;
} HotSendGuiIdleProgress;

typedef struct HotSendGuiIdleDone {
    HotSendGuiWidgets *Widgets;
    int ExitCode;
} HotSendGuiIdleDone;

typedef struct HotSendGuiWorkerJob {
    HotSendGuiWidgets *Widgets;
    char *HostOrDevice;
    char *FilePath;
    char *BoardPath;
    int PortOrBaud;
    int UseSerial;
    int NoReboot;
    int NoBackup;
    int NoPreallocate;
    int DryRun;
    int SteadyThrottle;
} HotSendGuiWorkerJob;

static gboolean HotSendGuiIdleAppendLog(gpointer UserData) {
    HotSendGuiIdleLog *Item = (HotSendGuiIdleLog *)UserData;

    if (Item->Refresh) {
        HotSendGuiRefreshLog(Item->Widgets, Item->Message);
    } else {
        HotSendGuiAppendLog(Item->Widgets, Item->Message);
    }
    g_free(Item->Message);
    g_free(Item);
    return G_SOURCE_REMOVE;
}

static gboolean HotSendGuiIdleSetProgress(gpointer UserData) {
    HotSendGuiIdleProgress *Item = (HotSendGuiIdleProgress *)UserData;
    double Fraction = 0.0;
    char Text[64];

    if (Item->Total > 0) {
        Fraction = (double)Item->Position / (double)Item->Total;
    }
    gtk_progress_bar_set_fraction(Item->Widgets->ProgressBar, Fraction);
    snprintf(Text, sizeof(Text), "%u/%u (%u KB)", (unsigned)Item->Position,
             (unsigned)Item->Total, (unsigned)(Item->SentBytes / 1024u));
    gtk_progress_bar_set_text(Item->Widgets->ProgressBar, Text);
    gtk_label_set_text(Item->Widgets->StatusLabel, Text);
    g_free(Item);
    return G_SOURCE_REMOVE;
}

static gboolean HotSendGuiIdleFinish(gpointer UserData) {
    HotSendGuiIdleDone *Item = (HotSendGuiIdleDone *)UserData;
    char Status[64];

    Item->Widgets->TransferRunning = 0;
    Item->Widgets->WorkerThread = NULL;
    gtk_widget_set_sensitive(GTK_WIDGET(Item->Widgets->StartButton), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(Item->Widgets->AbortButton), FALSE);
    if (Item->ExitCode == HotSendExitOk) {
        snprintf(Status, sizeof(Status), "OK");
    } else {
        snprintf(Status, sizeof(Status), "exit %d", Item->ExitCode);
    }
    gtk_label_set_text(Item->Widgets->StatusLabel, Status);
    g_free(Item);
    return G_SOURCE_REMOVE;
}

static void HotSendGuiWorkerLog(const char *Message, void *UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    HotSendGuiIdleLog *Item;

    Item = g_new0(HotSendGuiIdleLog, 1);
    Item->Widgets = Widgets;
    Item->Message = g_strdup(Message != NULL ? Message : "");
    Item->Refresh = HotSendLogLineIsProgress(Item->Message);
    g_idle_add(HotSendGuiIdleAppendLog, Item);
}

static void HotSendGuiWorkerProgress(uint32_t Position, uint32_t Total,
                                     uint32_t SentBytes, void *UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    HotSendGuiIdleProgress *Item;

    Item = g_new0(HotSendGuiIdleProgress, 1);
    Item->Widgets = Widgets;
    Item->Position = Position;
    Item->Total = Total;
    Item->SentBytes = SentBytes;
    g_idle_add(HotSendGuiIdleSetProgress, Item);
}

static gpointer HotSendGuiWorkerThreadMain(gpointer UserData) {
    HotSendGuiWorkerJob *Job = (HotSendGuiWorkerJob *)UserData;
    HotSendTransferRequest Request;
    HotSendGuiIdleDone *Done;
    int ExitCode;

    HotSendTransferRequestInit(&Request);
    Request.LocalFilePath = Job->FilePath;
    Request.BoardTargetPath = Job->BoardPath;
    Request.NoReboot = Job->NoReboot;
    Request.NoBackup = Job->NoBackup;
    Request.NoPreallocate = Job->NoPreallocate;
    Request.DryRun = Job->DryRun;
    Request.SteadyThrottle = Job->SteadyThrottle;
    Request.AckTimeoutSeconds = 120.0;
    Request.DataTimeoutSeconds = 180.0;
    Request.Log = HotSendGuiWorkerLog;
    Request.Progress = HotSendGuiWorkerProgress;
    Request.UserData = Job->Widgets;
    Request.CancelFlag = &Job->Widgets->CancelRequested;
    if (Job->UseSerial) {
        Request.SerialDevicePath = Job->HostOrDevice;
        Request.BaudRate = Job->PortOrBaud > 0 ? Job->PortOrBaud : 115200;
        ExitCode = HotSendTransferOverSerial(&Request);
    } else {
        Request.Host = Job->HostOrDevice;
        Request.TcpPort = Job->PortOrBaud;
        ExitCode = HotSendTransferOverTcp(&Request);
    }

    Done = g_new0(HotSendGuiIdleDone, 1);
    Done->Widgets = Job->Widgets;
    Done->ExitCode = ExitCode;
    g_idle_add(HotSendGuiIdleFinish, Done);

    g_free(Job->HostOrDevice);
    g_free(Job->FilePath);
    g_free(Job->BoardPath);
    g_free(Job);
    return NULL;
}

void HotSendGuiWorkerRequestCancel(HotSendGuiWidgets *Widgets) {
    if (Widgets == NULL) {
        return;
    }
    Widgets->CancelRequested = 1;
    HotSendGuiAppendLog(Widgets, "abort requested…");
}

void HotSendGuiWorkerStart(HotSendGuiWidgets *Widgets) {
    HotSendGuiWorkerJob *Job;
    const char *Endpoint;
    const char *FilePath;
    const char *PortText;
    GtkStringObject *PathObject;
    const char *BoardPath;
    int UseSerial;

    if (Widgets == NULL || Widgets->TransferRunning) {
        return;
    }
    UseSerial = gtk_check_button_get_active(Widgets->SerialRadio) ? 1 : 0;
    if (UseSerial) {
        Endpoint = HotSendGuiDropDownSelectedText(Widgets->DeviceDropDown);
        PortText = HotSendGuiDropDownSelectedText(Widgets->BaudDropDown);
    } else {
        Endpoint = gtk_editable_get_text(GTK_EDITABLE(Widgets->HostEntry));
        PortText = gtk_editable_get_text(GTK_EDITABLE(Widgets->TcpPortEntry));
    }
    FilePath = gtk_editable_get_text(GTK_EDITABLE(Widgets->FileEntry));
    PathObject = GTK_STRING_OBJECT(
        gtk_drop_down_get_selected_item(Widgets->BoardPathDropDown));
    BoardPath = PathObject != NULL ? gtk_string_object_get_string(PathObject)
                                   : "BLOCKS:Kernel.elf";
    if (Endpoint == NULL || Endpoint[0] == 0 || FilePath == NULL || FilePath[0] == 0) {
        HotSendGuiAppendLog(Widgets,
                            Widgets->Language == 1
                                ? (UseSerial ? "需要设备和文件" : "需要主机和文件")
                                : (UseSerial ? "need device and file" : "need host and file"));
        return;
    }

    Widgets->CancelRequested = 0;
    Widgets->TransferRunning = 1;
    HotSendGuiClearLog(Widgets);
    gtk_widget_set_sensitive(GTK_WIDGET(Widgets->StartButton), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(Widgets->AbortButton), TRUE);
    gtk_progress_bar_set_fraction(Widgets->ProgressBar, 0.0);
    gtk_progress_bar_set_text(Widgets->ProgressBar, "");
    gtk_label_set_text(Widgets->StatusLabel, "Starting…");

    Job = g_new0(HotSendGuiWorkerJob, 1);
    Job->Widgets = Widgets;
    Job->HostOrDevice = g_strdup(Endpoint);
    Job->FilePath = g_strdup(FilePath);
    Job->BoardPath = g_strdup(BoardPath);
    Job->UseSerial = UseSerial;
    if (UseSerial) {
        Job->PortOrBaud = PortText != NULL && PortText[0] != 0 ? atoi(PortText) : 115200;
    } else {
        Job->PortOrBaud =
            PortText != NULL && PortText[0] != 0 ? atoi(PortText)
                                                : (int)HotSendDefaultTcpPort;
    }
    Job->NoReboot = gtk_check_button_get_active(Widgets->NoRebootCheck) ? 1 : 0;
    Job->NoBackup = gtk_check_button_get_active(Widgets->NoBackupCheck) ? 1 : 0;
    Job->NoPreallocate =
        gtk_check_button_get_active(Widgets->NoPreallocateCheck) ? 1 : 0;
    Job->DryRun = gtk_check_button_get_active(Widgets->DryRunCheck) ? 1 : 0;
    Job->SteadyThrottle =
        gtk_check_button_get_active(Widgets->SteadyThrottleCheck) ? 1 : 0;

    Widgets->WorkerThread =
        g_thread_new("hotsend-transfer", HotSendGuiWorkerThreadMain, Job);
}
