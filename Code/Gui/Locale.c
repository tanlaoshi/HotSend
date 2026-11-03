/*
 * Locale.c — Gui 中/英文案
 */
#include "Locale.h"

#include <stdio.h>
#include <string.h>

typedef struct HotSendGuiLocalePack {
    const char *BrandTitle;
    const char *WindowTitle;
    const char *Serial;
    const char *Tcp;
    const char *Device;
    const char *Baud;
    const char *Host;
    const char *TcpPort;
    const char *RefreshDevices;
    const char *File;
    const char *Browse;
    const char *BoardPath;
    const char *NoReboot;
    const char *NoBackup;
    const char *DryRun;
    const char *SteadyThrottle;
    const char *Start;
    const char *Abort;
    const char *Idle;
    const char *Log;
    const char *BrowseTitle;
} HotSendGuiLocalePack;

static const HotSendGuiLocalePack HotSendGuiLocaleEnglish = {
    "HotSend",
    "HotSend",
    "Serial",
    "TCP",
    "Device",
    "Baud",
    "Host",
    "TCP port",
    "Refresh",
    "File",
    "Browse",
    "Board path",
    "No reboot",
    "Skip backup",
    "Preview only (no write)",
    "Steady throttle (=pace 500)",
    "Start",
    "Abort",
    "Idle",
    "Log",
    "Select file to send",
};

static const HotSendGuiLocalePack HotSendGuiLocaleChinese = {
    "热传",
    "热传",
    "串口",
    "TCP",
    "设备",
    "波特率",
    "主机",
    "TCP 端口",
    "刷新",
    "文件",
    "浏览",
    "板端路径",
    "不重启",
    "跳过备份",
    "仅预览差量(不写盘)",
    "稳妥节流(=pace 500)",
    "开始",
    "中止",
    "空闲",
    "日志",
    "选择要发送的文件",
};

void HotSendGuiApplyLocale(HotSendGuiWidgets *Widgets, int Language) {
    const HotSendGuiLocalePack *Pack;

    if (Widgets == NULL) {
        return;
    }
    Pack = Language == HotSendGuiLanguageChinese ? &HotSendGuiLocaleChinese
                                                 : &HotSendGuiLocaleEnglish;
    Widgets->Language = Language;
    Widgets->BrowseDialogTitle = Pack->BrowseTitle;
    if (Widgets->BrandTitleLabel != NULL) {
        char Markup[96];

        snprintf(Markup, sizeof(Markup),
                 "<span size='xx-large' weight='bold'>%s</span>", Pack->BrandTitle);
        gtk_label_set_markup(Widgets->BrandTitleLabel, Markup);
    }
    if (Widgets->Window != NULL) {
        gtk_window_set_title(Widgets->Window, Pack->WindowTitle);
    }
    gtk_check_button_set_label(Widgets->SerialRadio, Pack->Serial);
    gtk_check_button_set_label(Widgets->TcpRadio, Pack->Tcp);
    if (Widgets->DeviceLabel != NULL) {
        gtk_label_set_text(Widgets->DeviceLabel, Pack->Device);
    }
    if (Widgets->BaudLabel != NULL) {
        gtk_label_set_text(Widgets->BaudLabel, Pack->Baud);
    }
    if (Widgets->HostLabel != NULL) {
        gtk_label_set_text(Widgets->HostLabel, Pack->Host);
    }
    if (Widgets->TcpPortLabel != NULL) {
        gtk_label_set_text(Widgets->TcpPortLabel, Pack->TcpPort);
    }
    if (Widgets->RefreshDevicesButton != NULL) {
        gtk_button_set_label(Widgets->RefreshDevicesButton, Pack->RefreshDevices);
    }
    if (Widgets->FileLabel != NULL) {
        gtk_label_set_text(Widgets->FileLabel, Pack->File);
    }
    if (Widgets->BrowseButton != NULL) {
        gtk_button_set_label(Widgets->BrowseButton, Pack->Browse);
    }
    if (Widgets->BoardPathLabel != NULL) {
        gtk_label_set_text(Widgets->BoardPathLabel, Pack->BoardPath);
    }
    gtk_check_button_set_label(Widgets->NoRebootCheck, Pack->NoReboot);
    gtk_check_button_set_label(Widgets->NoBackupCheck, Pack->NoBackup);
    gtk_check_button_set_label(Widgets->DryRunCheck, Pack->DryRun);
    gtk_check_button_set_label(Widgets->SteadyThrottleCheck, Pack->SteadyThrottle);
    gtk_button_set_label(Widgets->StartButton, Pack->Start);
    gtk_button_set_label(Widgets->AbortButton, Pack->Abort);
    if (!Widgets->TransferRunning) {
        gtk_label_set_text(Widgets->StatusLabel, Pack->Idle);
    }
    if (Widgets->LogFrame != NULL) {
        gtk_frame_set_label(Widgets->LogFrame, Pack->Log);
    }
}
