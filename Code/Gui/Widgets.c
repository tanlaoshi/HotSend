/*
 * Widgets.c — 日志追加（窗体组装见 Layout.c）
 */
#include "Widgets.h"

void HotSendGuiAppendLog(HotSendGuiWidgets *Widgets, const char *Line) {
    GtkTextIter End;

    if (Widgets == NULL || Widgets->LogBuffer == NULL || Line == NULL) {
        return;
    }
    gtk_text_buffer_get_end_iter(Widgets->LogBuffer, &End);
    gtk_text_buffer_insert(Widgets->LogBuffer, &End, Line, -1);
    gtk_text_buffer_insert(Widgets->LogBuffer, &End, "\n", 1);
}
