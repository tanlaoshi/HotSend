/*
 * Layout.c — 窗体组装（语言 / Serial·TCP / 串口下拉）
 */
#include "Widgets.h"

#include "Icon.h"
#include "LayoutSupport.h"
#include "Locale.h"
#include "SerialPorts.h"

#include <stdlib.h>
#include <string.h>

static GtkWidget *HotSendGuiBuildLabelDropRow(GtkLabel **OutLabel, const char *LabelText,
                                              GtkDropDown **OutDrop, GListModel *Model) {
    GtkWidget *Row;

    Row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    *OutLabel = GTK_LABEL(gtk_label_new(LabelText));
    gtk_box_append(GTK_BOX(Row), GTK_WIDGET(*OutLabel));
    *OutDrop = GTK_DROP_DOWN(gtk_drop_down_new(Model, NULL));
    gtk_widget_set_hexpand(GTK_WIDGET(*OutDrop), TRUE);
    gtk_box_append(GTK_BOX(Row), GTK_WIDGET(*OutDrop));
    return Row;
}

static GtkWidget *HotSendGuiBuildLabeledEntry(GtkLabel **OutLabel, const char *LabelText,
                                              GtkEntry **OutEntry, const char *Initial) {
    GtkWidget *Row;

    Row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    *OutLabel = GTK_LABEL(gtk_label_new(LabelText));
    gtk_box_append(GTK_BOX(Row), GTK_WIDGET(*OutLabel));
    *OutEntry = GTK_ENTRY(gtk_entry_new());
    if (Initial != NULL) {
        gtk_editable_set_text(GTK_EDITABLE(*OutEntry), Initial);
    }
    gtk_widget_set_hexpand(GTK_WIDGET(*OutEntry), TRUE);
    gtk_box_append(GTK_BOX(Row), GTK_WIDGET(*OutEntry));
    return Row;
}

GtkWidget *HotSendGuiBuildWindow(HotSendGuiWidgets *Widgets) {
    GtkWidget *Window;
    GtkWidget *Outer;
    GtkWidget *LanguageRow;
    GtkWidget *TransportRow;
    GtkWidget *DeviceRow;
    GtkWidget *FileRow;
    GtkWidget *OptionsRow;
    GtkWidget *Actions;
    GtkWidget *LogScroll;
    const char *PathChoices[] = {"BLOCKS:Kernel.elf", "D:Kernel.elf", NULL};
    const char *EmptyDevices[] = {NULL};
    const char *Lang = getenv("LANG");
    int PreferChinese = Lang != NULL && strncmp(Lang, "zh", 2) == 0;

    Window = gtk_application_window_new(Widgets->Application);
    Widgets->Window = GTK_WINDOW(Window);
    gtk_window_set_title(Widgets->Window, "HotSend");
    gtk_window_set_default_size(Widgets->Window, 640, 600);
    HotSendGuiApplyWindowIcon(Widgets->Window);

    Outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(Outer, 16);
    gtk_widget_set_margin_bottom(Outer, 16);
    gtk_widget_set_margin_start(Outer, 16);
    gtk_widget_set_margin_end(Outer, 16);
    gtk_window_set_child(Widgets->Window, Outer);

    LanguageRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    Widgets->BrandTitleLabel = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_markup(Widgets->BrandTitleLabel,
                         "<span size='xx-large' weight='bold'>HotSend</span>");
    gtk_widget_set_halign(GTK_WIDGET(Widgets->BrandTitleLabel), GTK_ALIGN_START);
    gtk_widget_set_hexpand(GTK_WIDGET(Widgets->BrandTitleLabel), TRUE);
    gtk_box_append(GTK_BOX(LanguageRow), GTK_WIDGET(Widgets->BrandTitleLabel));
    Widgets->EnglishRadio = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("English"));
    Widgets->ChineseRadio = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("中文"));
    gtk_check_button_set_group(Widgets->ChineseRadio, Widgets->EnglishRadio);
    gtk_check_button_set_active(
        PreferChinese ? Widgets->ChineseRadio : Widgets->EnglishRadio, TRUE);
    g_signal_connect(Widgets->ChineseRadio, "toggled",
                     G_CALLBACK(HotSendGuiOnLanguageToggled), Widgets);
    gtk_box_append(GTK_BOX(LanguageRow), GTK_WIDGET(Widgets->EnglishRadio));
    gtk_box_append(GTK_BOX(LanguageRow), GTK_WIDGET(Widgets->ChineseRadio));
    gtk_box_append(GTK_BOX(Outer), LanguageRow);

    TransportRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    Widgets->SerialRadio = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Serial"));
    Widgets->TcpRadio = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("TCP"));
    gtk_check_button_set_group(Widgets->TcpRadio, Widgets->SerialRadio);
    gtk_check_button_set_active(Widgets->SerialRadio, TRUE);
    g_signal_connect(Widgets->SerialRadio, "toggled",
                     G_CALLBACK(HotSendGuiOnTransportToggled), Widgets);
    gtk_box_append(GTK_BOX(TransportRow), GTK_WIDGET(Widgets->SerialRadio));
    gtk_box_append(GTK_BOX(TransportRow), GTK_WIDGET(Widgets->TcpRadio));
    gtk_box_append(GTK_BOX(Outer), TransportRow);

    Widgets->TransportStack = GTK_STACK(gtk_stack_new());
    gtk_stack_set_transition_type(Widgets->TransportStack,
                                  GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    Widgets->SerialPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    DeviceRow = HotSendGuiBuildLabelDropRow(
        &Widgets->DeviceLabel, "Device", &Widgets->DeviceDropDown,
        G_LIST_MODEL(gtk_string_list_new(EmptyDevices)));
    Widgets->RefreshDevicesButton = GTK_BUTTON(gtk_button_new_with_label("Refresh"));
    g_signal_connect(Widgets->RefreshDevicesButton, "clicked",
                     G_CALLBACK(HotSendGuiOnRefreshDevices), Widgets);
    gtk_box_append(GTK_BOX(DeviceRow), GTK_WIDGET(Widgets->RefreshDevicesButton));
    gtk_box_append(GTK_BOX(Widgets->SerialPage), DeviceRow);
    gtk_box_append(GTK_BOX(Widgets->SerialPage),
                   HotSendGuiBuildLabelDropRow(
                       &Widgets->BaudLabel, "Baud", &Widgets->BaudDropDown,
                       G_LIST_MODEL(HotSendGuiSerialBaudListNew())));
    Widgets->TcpPage = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_append(GTK_BOX(Widgets->TcpPage),
                   HotSendGuiBuildLabeledEntry(&Widgets->HostLabel, "Host",
                                               &Widgets->HostEntry, "192.168.31.47"));
    gtk_box_append(GTK_BOX(Widgets->TcpPage),
                   HotSendGuiBuildLabeledEntry(&Widgets->TcpPortLabel, "TCP port",
                                               &Widgets->TcpPortEntry, "26992"));
    gtk_stack_add_named(Widgets->TransportStack, Widgets->SerialPage, "serial");
    gtk_stack_add_named(Widgets->TransportStack, Widgets->TcpPage, "tcp");
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->TransportStack));
    HotSendGuiShowTransportPage(Widgets, 1);

    FileRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    Widgets->FileLabel = GTK_LABEL(gtk_label_new("File"));
    gtk_box_append(GTK_BOX(FileRow), GTK_WIDGET(Widgets->FileLabel));
    Widgets->FileEntry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(Widgets->FileEntry),
                          "/home/tank/Blocks/Kernel/Build/HAL/X64/Kernel.elf");
    gtk_widget_set_hexpand(GTK_WIDGET(Widgets->FileEntry), TRUE);
    gtk_box_append(GTK_BOX(FileRow), GTK_WIDGET(Widgets->FileEntry));
    Widgets->BrowseButton = GTK_BUTTON(gtk_button_new_with_label("Browse"));
    g_signal_connect(Widgets->BrowseButton, "clicked", G_CALLBACK(HotSendGuiOnBrowseFile),
                     Widgets);
    gtk_box_append(GTK_BOX(FileRow), GTK_WIDGET(Widgets->BrowseButton));
    gtk_box_append(GTK_BOX(Outer), FileRow);

    Widgets->BoardPathLabel = GTK_LABEL(gtk_label_new("Board path"));
    gtk_widget_set_halign(GTK_WIDGET(Widgets->BoardPathLabel), GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->BoardPathLabel));
    Widgets->BoardPathDropDown = GTK_DROP_DOWN(
        gtk_drop_down_new(G_LIST_MODEL(gtk_string_list_new(PathChoices)), NULL));
    gtk_drop_down_set_selected(Widgets->BoardPathDropDown, 1);
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->BoardPathDropDown));

    OptionsRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    Widgets->NoRebootCheck = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("No reboot"));
    gtk_check_button_set_active(Widgets->NoRebootCheck, TRUE);
    Widgets->NoBackupCheck =
        GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Skip backup"));
    Widgets->DryRunCheck = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Dry run"));
    Widgets->SteadyThrottleCheck =
        GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Steady throttle"));
    gtk_box_append(GTK_BOX(OptionsRow), GTK_WIDGET(Widgets->NoRebootCheck));
    gtk_box_append(GTK_BOX(OptionsRow), GTK_WIDGET(Widgets->NoBackupCheck));
    gtk_box_append(GTK_BOX(OptionsRow), GTK_WIDGET(Widgets->DryRunCheck));
    gtk_box_append(GTK_BOX(OptionsRow), GTK_WIDGET(Widgets->SteadyThrottleCheck));
    gtk_box_append(GTK_BOX(Outer), OptionsRow);

    Actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    Widgets->StartButton = GTK_BUTTON(gtk_button_new_with_label("Start"));
    Widgets->AbortButton = GTK_BUTTON(gtk_button_new_with_label("Abort"));
    gtk_widget_set_sensitive(GTK_WIDGET(Widgets->AbortButton), FALSE);
    g_signal_connect(Widgets->StartButton, "clicked", G_CALLBACK(HotSendGuiOnStart),
                     Widgets);
    g_signal_connect(Widgets->AbortButton, "clicked", G_CALLBACK(HotSendGuiOnAbort),
                     Widgets);
    gtk_box_append(GTK_BOX(Actions), GTK_WIDGET(Widgets->StartButton));
    gtk_box_append(GTK_BOX(Actions), GTK_WIDGET(Widgets->AbortButton));
    gtk_box_append(GTK_BOX(Outer), Actions);

    Widgets->ProgressBar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_progress_bar_set_show_text(Widgets->ProgressBar, TRUE);
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->ProgressBar));
    Widgets->StatusLabel = GTK_LABEL(gtk_label_new("Idle"));
    gtk_widget_set_halign(GTK_WIDGET(Widgets->StatusLabel), GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->StatusLabel));

    Widgets->LogFrame = GTK_FRAME(gtk_frame_new("Log"));
    LogScroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(LogScroll), 160);
    gtk_widget_set_vexpand(LogScroll, TRUE);
    Widgets->LogView = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(Widgets->LogView, FALSE);
    gtk_text_view_set_monospace(Widgets->LogView, TRUE);
    Widgets->LogBuffer = gtk_text_view_get_buffer(Widgets->LogView);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(LogScroll),
                                  GTK_WIDGET(Widgets->LogView));
    gtk_frame_set_child(Widgets->LogFrame, LogScroll);
    gtk_widget_set_vexpand(GTK_WIDGET(Widgets->LogFrame), TRUE);
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->LogFrame));

    HotSendGuiApplyConfig(Widgets);
    HotSendGuiApplyLocale(Widgets, PreferChinese ? HotSendGuiLanguageChinese
                                                 : HotSendGuiLanguageEnglish);
    return Window;
}
