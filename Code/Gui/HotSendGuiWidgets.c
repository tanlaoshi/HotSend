/*
 * HotSendGuiWidgets.c — GTK4 一屏布局（PR-3）
 */
#include "HotSendGuiWidgets.h"

#include "HotSendGuiWorker.h"

void HotSendGuiAppendLog(HotSendGuiWidgets *Widgets, const char *Line) {
    GtkTextIter End;

    if (Widgets == NULL || Widgets->LogBuffer == NULL || Line == NULL) {
        return;
    }
    gtk_text_buffer_get_end_iter(Widgets->LogBuffer, &End);
    gtk_text_buffer_insert(Widgets->LogBuffer, &End, Line, -1);
    gtk_text_buffer_insert(Widgets->LogBuffer, &End, "\n", 1);
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

static void HotSendGuiOnBrowseFile(GtkButton *Button, gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    GtkFileDialog *Dialog;

    (void)Button;
    Dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(Dialog, "Select file to send");
    gtk_file_dialog_open(Dialog, Widgets->Window, NULL, HotSendGuiOnBrowseFinish,
                         Widgets);
    g_object_unref(Dialog);
}

static void HotSendGuiOnStart(GtkButton *Button, gpointer UserData) {
    (void)Button;
    HotSendGuiWorkerStart((HotSendGuiWidgets *)UserData);
}

static void HotSendGuiOnAbort(GtkButton *Button, gpointer UserData) {
    (void)Button;
    HotSendGuiWorkerRequestCancel((HotSendGuiWidgets *)UserData);
}

GtkWidget *HotSendGuiBuildWindow(HotSendGuiWidgets *Widgets) {
    GtkWidget *Window;
    GtkWidget *Outer;
    GtkWidget *Title;
    GtkWidget *HostRow;
    GtkWidget *PortRow;
    GtkWidget *FileRow;
    GtkWidget *Browse;
    GtkWidget *PathLabel;
    GtkWidget *OptionsRow;
    GtkWidget *Actions;
    GtkWidget *LogFrame;
    GtkWidget *LogScroll;
    const char *PathChoices[] = {"BLOCKS:Kernel.elf", "D:Kernel.elf", NULL};
    GtkStringList *PathList;

    Window = gtk_application_window_new(Widgets->Application);
    Widgets->Window = GTK_WINDOW(Window);
    gtk_window_set_title(Widgets->Window, "HotSend");
    gtk_window_set_default_size(Widgets->Window, 640, 520);

    Outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(Outer, 16);
    gtk_widget_set_margin_bottom(Outer, 16);
    gtk_widget_set_margin_start(Outer, 16);
    gtk_widget_set_margin_end(Outer, 16);
    gtk_window_set_child(Widgets->Window, Outer);

    Title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(Title),
                         "<span size='xx-large' weight='bold'>HotSend</span>");
    gtk_widget_set_halign(Title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(Outer), Title);

    Widgets->UseSerialCheck =
        GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Use serial (not TCP)"));
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->UseSerialCheck));

    HostRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(HostRow), gtk_label_new("Host / Device"));
    Widgets->HostEntry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(Widgets->HostEntry), "192.168.31.47");
    gtk_widget_set_hexpand(GTK_WIDGET(Widgets->HostEntry), TRUE);
    gtk_box_append(GTK_BOX(HostRow), GTK_WIDGET(Widgets->HostEntry));
    gtk_box_append(GTK_BOX(Outer), HostRow);

    PortRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(PortRow), gtk_label_new("TCP port / Baud"));
    Widgets->PortEntry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(Widgets->PortEntry), "26992");
    gtk_box_append(GTK_BOX(PortRow), GTK_WIDGET(Widgets->PortEntry));
    gtk_box_append(GTK_BOX(Outer), PortRow);

    FileRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(FileRow), gtk_label_new("File"));
    Widgets->FileEntry = GTK_ENTRY(gtk_entry_new());
    gtk_editable_set_text(GTK_EDITABLE(Widgets->FileEntry),
                          "/home/tank/Blocks/Kernel/Build/HAL/X64/Kernel.elf");
    gtk_widget_set_hexpand(GTK_WIDGET(Widgets->FileEntry), TRUE);
    gtk_box_append(GTK_BOX(FileRow), GTK_WIDGET(Widgets->FileEntry));
    Browse = gtk_button_new_with_label("Browse");
    g_signal_connect(Browse, "clicked", G_CALLBACK(HotSendGuiOnBrowseFile), Widgets);
    gtk_box_append(GTK_BOX(FileRow), Browse);
    gtk_box_append(GTK_BOX(Outer), FileRow);

    PathLabel = gtk_label_new("Board path");
    gtk_widget_set_halign(PathLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(Outer), PathLabel);
    PathList = gtk_string_list_new(PathChoices);
    Widgets->BoardPathDropDown = GTK_DROP_DOWN(gtk_drop_down_new(G_LIST_MODEL(PathList), NULL));
    gtk_drop_down_set_selected(Widgets->BoardPathDropDown, 1);
    gtk_box_append(GTK_BOX(Outer), GTK_WIDGET(Widgets->BoardPathDropDown));

    OptionsRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    Widgets->NoRebootCheck = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("No reboot"));
    gtk_check_button_set_active(Widgets->NoRebootCheck, TRUE);
    Widgets->NoBackupCheck =
        GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Skip backup"));
    gtk_box_append(GTK_BOX(OptionsRow), GTK_WIDGET(Widgets->NoRebootCheck));
    gtk_box_append(GTK_BOX(OptionsRow), GTK_WIDGET(Widgets->NoBackupCheck));
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

    LogFrame = gtk_frame_new("Log");
    LogScroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(LogScroll), 160);
    gtk_widget_set_vexpand(LogScroll, TRUE);
    Widgets->LogView = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(Widgets->LogView, FALSE);
    gtk_text_view_set_monospace(Widgets->LogView, TRUE);
    Widgets->LogBuffer = gtk_text_view_get_buffer(Widgets->LogView);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(LogScroll),
                                  GTK_WIDGET(Widgets->LogView));
    gtk_frame_set_child(GTK_FRAME(LogFrame), LogScroll);
    gtk_widget_set_vexpand(LogFrame, TRUE);
    gtk_box_append(GTK_BOX(Outer), LogFrame);

    return Window;
}
