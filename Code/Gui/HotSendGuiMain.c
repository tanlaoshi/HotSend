/*
 * HotSendGuiMain.c — GTK4 应用入口（PR-3）
 */
#include "HotSendGuiWidgets.h"

#include <gtk/gtk.h>
#include <string.h>

static void HotSendGuiOnActivate(GtkApplication *Application, gpointer UserData) {
    HotSendGuiWidgets *Widgets = (HotSendGuiWidgets *)UserData;
    GtkWidget *Window;

    Widgets->Application = Application;
    Window = HotSendGuiBuildWindow(Widgets);
    gtk_window_present(GTK_WINDOW(Window));
}

int main(int ArgumentCount, char **ArgumentVector) {
    HotSendGuiWidgets Widgets;
    GtkApplication *Application;
    int Status;

    memset(&Widgets, 0, sizeof(Widgets));
    Application = gtk_application_new("com.blocks.hotsend", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(Application, "activate", G_CALLBACK(HotSendGuiOnActivate), &Widgets);
    Status = g_application_run(G_APPLICATION(Application), ArgumentCount, ArgumentVector);
    g_object_unref(Application);
    return Status;
}
