/*
 * Widgets.c — 日志追加 / 同行刷新 / 清空（窗体组装见 Layout.c）
 */
#include "Widgets.h"

#include <string.h>

int HotSendLogLineIsProgress(const char *Line) {
    if (Line == NULL) {
        return 0;
    }
    if (strncmp(Line, "sent ", 5) == 0) {
        return 1;
    }
    if (strncmp(Line, "waiting for board line", 22) == 0) {
        return 1;
    }
    if (strncmp(Line, "waiting for KSEND", 17) == 0) {
        return 1;
    }
    if (strncmp(Line, "progress ", 9) == 0) {
        return 1;
    }
    return 0;
}

static void HotSendGuiClearProgressMark(HotSendGuiWidgets *Widgets) {
    if (Widgets == NULL || Widgets->LogBuffer == NULL ||
        Widgets->LogProgressMark == NULL) {
        return;
    }
    gtk_text_buffer_delete_mark(Widgets->LogBuffer, Widgets->LogProgressMark);
    Widgets->LogProgressMark = NULL;
}

static void HotSendGuiScrollLogToEnd(HotSendGuiWidgets *Widgets) {
    GtkTextIter End;
    GtkTextMark *Insert;

    if (Widgets == NULL || Widgets->LogBuffer == NULL || Widgets->LogView == NULL) {
        return;
    }
    gtk_text_buffer_get_end_iter(Widgets->LogBuffer, &End);
    Insert = gtk_text_buffer_get_insert(Widgets->LogBuffer);
    gtk_text_buffer_place_cursor(Widgets->LogBuffer, &End);
    gtk_text_view_scroll_mark_onscreen(Widgets->LogView, Insert);
}

void HotSendGuiAppendLog(HotSendGuiWidgets *Widgets, const char *Line) {
    GtkTextIter End;

    if (Widgets == NULL || Widgets->LogBuffer == NULL || Line == NULL) {
        return;
    }
    HotSendGuiClearProgressMark(Widgets);
    gtk_text_buffer_get_end_iter(Widgets->LogBuffer, &End);
    gtk_text_buffer_insert(Widgets->LogBuffer, &End, Line, -1);
    gtk_text_buffer_insert(Widgets->LogBuffer, &End, "\n", 1);
    HotSendGuiScrollLogToEnd(Widgets);
}

void HotSendGuiRefreshLog(HotSendGuiWidgets *Widgets, const char *Line) {
    GtkTextIter Start;
    GtkTextIter End;

    if (Widgets == NULL || Widgets->LogBuffer == NULL || Line == NULL) {
        return;
    }
    if (Widgets->LogProgressMark == NULL) {
        gtk_text_buffer_get_end_iter(Widgets->LogBuffer, &Start);
        Widgets->LogProgressMark = gtk_text_buffer_create_mark(
            Widgets->LogBuffer, "hotsend-progress", &Start, TRUE);
        gtk_text_buffer_insert(Widgets->LogBuffer, &Start, Line, -1);
        gtk_text_buffer_insert(Widgets->LogBuffer, &Start, "\n", 1);
        HotSendGuiScrollLogToEnd(Widgets);
        return;
    }
    gtk_text_buffer_get_iter_at_mark(Widgets->LogBuffer, &Start,
                                     Widgets->LogProgressMark);
    End = Start;
    if (!gtk_text_iter_forward_line(&End)) {
        gtk_text_buffer_get_end_iter(Widgets->LogBuffer, &End);
    }
    gtk_text_buffer_delete(Widgets->LogBuffer, &Start, &End);
    gtk_text_buffer_get_iter_at_mark(Widgets->LogBuffer, &Start,
                                     Widgets->LogProgressMark);
    gtk_text_buffer_insert(Widgets->LogBuffer, &Start, Line, -1);
    gtk_text_buffer_insert(Widgets->LogBuffer, &Start, "\n", 1);
    HotSendGuiScrollLogToEnd(Widgets);
}

void HotSendGuiClearLog(HotSendGuiWidgets *Widgets) {
    if (Widgets == NULL || Widgets->LogBuffer == NULL) {
        return;
    }
    HotSendGuiClearProgressMark(Widgets);
    gtk_text_buffer_set_text(Widgets->LogBuffer, "", -1);
}
