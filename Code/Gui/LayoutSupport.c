/*
 * LayoutSupport.c — 配置预填 / 浏览 / 传输页切换
 */
#include "Widgets.h"

#include "Config.h"
#include "Constants.h"
#include "Locale.h"
#include "SerialPorts.h"
#include "Worker.h"

#include <stdio.h>
#include <string.h>

void HotSendGuiShowTransportPage(HotSendGuiWidgets *Widgets, int Serial) {
    if (Widgets == NULL || Widgets->TransportStack == NULL) {
        return;
    }
    gtk_stack_set_visible_child(Widgets->TransportStack,
                                Serial ? Widgets->SerialPage : Widgets->TcpPage);
}

void HotSendGuiOnTransportToggled(GtkCheckButton *Button, gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;

    (void)Button;
    HotSendGuiShowTransportPage(
        Widgets, Widgets != NULL && gtk_check_button_get_active(Widgets->SerialRadio));
}

void HotSendGuiOnLanguageToggled(GtkCheckButton *Button, gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;

    (void)Button;
    if (Widgets == NULL) {
        return;
    }
    HotSendGuiApplyLocale(Widgets, gtk_check_button_get_active(Widgets->ChineseRadio)
                                        ? HotSendGuiLanguageChinese
                                        : HotSendGuiLanguageEnglish);
}

void HotSendGuiOnRefreshDevices(GtkButton *Button, gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    const char *Current;

    (void)Button;
    Current = HotSendGuiDropDownSelectedText(Widgets->DeviceDropDown);
    HotSendGuiSerialRefreshDevices(Widgets, Current);
}

void HotSendGuiApplyConfig(HotSendGuiWidgets *Widgets) {
    HotSendConfig Config;
    char NumberText[32];
    int UseSerial;

    if (Widgets == NULL || HotSendConfigLoad(&Config, NULL) != 0) {
        HotSendGuiSerialRefreshDevices(Widgets, "/dev/ttyUSB0");
        HotSendGuiSerialSelectBaud(Widgets, 115200);
        return;
    }
    HotSendGuiSerialRefreshDevices(
        Widgets, Config.HaveSerial ? Config.SerialDevicePath : "/dev/ttyUSB0");
    HotSendGuiSerialSelectBaud(Widgets, Config.BaudRate > 0 ? Config.BaudRate : 115200);
    if (Config.HaveHost) {
        gtk_editable_set_text(GTK_EDITABLE(Widgets->HostEntry), Config.Host);
        snprintf(NumberText, sizeof(NumberText), "%d",
                 Config.TcpPort > 0 ? Config.TcpPort : (int)HotSendDefaultTcpPort);
        gtk_editable_set_text(GTK_EDITABLE(Widgets->TcpPortEntry), NumberText);
    }
    UseSerial = Config.PreferSerial;
    if (UseSerial < 0) {
        UseSerial = Config.HaveSerial ? 1 : 0;
    }
    gtk_check_button_set_active(UseSerial ? Widgets->SerialRadio : Widgets->TcpRadio,
                                TRUE);
    HotSendGuiShowTransportPage(Widgets, UseSerial);
    if (Config.HaveFile) {
        gtk_editable_set_text(GTK_EDITABLE(Widgets->FileEntry), Config.LocalFilePath);
    }
    if (Config.HavePath) {
        gtk_drop_down_set_selected(Widgets->BoardPathDropDown,
                                   strcmp(Config.BoardTargetPath, "D:Kernel.elf") == 0
                                       ? 1
                                       : 0);
    }
    gtk_check_button_set_active(Widgets->NoRebootCheck, Config.NoReboot ? TRUE : FALSE);
    gtk_check_button_set_active(Widgets->NoBackupCheck, Config.NoBackup ? TRUE : FALSE);
    gtk_check_button_set_active(Widgets->NoPreallocateCheck,
                                Config.NoPreallocate ? TRUE : FALSE);
    gtk_check_button_set_active(Widgets->DryRunCheck, Config.DryRun ? TRUE : FALSE);
    gtk_check_button_set_active(Widgets->SteadyThrottleCheck,
                                Config.SteadyThrottle ? TRUE : FALSE);
    {
        char Note[576];

        snprintf(Note, sizeof(Note), "config: %s", Config.LoadedFrom);
        HotSendGuiAppendLog(Widgets, Note);
    }
}

static void HotSendGuiOnBrowseFinish(GObject *Source, GAsyncResult *Result,
                                     gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    GFile *File;
    char *Path;
    GError *Error = NULL;

    File = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(Source), Result, &Error);
    if (Error != NULL) {
        g_error_free(Error);
        return;
    }
    if (File == NULL) {
        return;
    }
    Path = g_file_get_path(File);
    if (Path != NULL) {
        gtk_editable_set_text(GTK_EDITABLE(Widgets->FileEntry), Path);
        g_free(Path);
    }
    g_object_unref(File);
}

void HotSendGuiOnBrowseFile(GtkButton *Button, gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    GtkFileDialog *Dialog;

    (void)Button;
    Dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(
        Dialog, Widgets->BrowseDialogTitle != NULL ? Widgets->BrowseDialogTitle
                                                   : "Select file to send");
    gtk_file_dialog_open(Dialog, Widgets->Window, NULL, HotSendGuiOnBrowseFinish,
                         Widgets);
    g_object_unref(Dialog);
}

void HotSendGuiOnStart(GtkButton *Button, gpointer UserData) {
    (void)Button;
    HotSendGuiWorkerStart((HotSendGuiWidgets *)UserData);
}

void HotSendGuiOnAbort(GtkButton *Button, gpointer UserData) {
    (void)Button;
    HotSendGuiWorkerRequestCancel((HotSendGuiWidgets *)UserData);
}
