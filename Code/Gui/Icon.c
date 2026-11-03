/*
 * Icon.c — 解析 Assets/icons 并设置窗口图标
 */
#include "Icon.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int HotSendGuiDirectoryLooksLikeAssetsIcons(const char *Path) {
    char Probe[640];

    if (Path == NULL || Path[0] == 0) {
        return 0;
    }
    snprintf(Probe, sizeof(Probe), "%s/hicolor/128x128/apps/hotsend.png", Path);
    return access(Probe, R_OK) == 0;
}

static int HotSendGuiResolveIconsDirectory(char *Out, size_t Capacity) {
    const char *Home;
    const char *Root;
    char Candidate[768];
    char Executable[512];
    ssize_t Length;

    Root = getenv("HOTSEND_ROOT");
    if (Root != NULL && Root[0] != 0) {
        snprintf(Candidate, sizeof(Candidate), "%s/Assets/icons", Root);
        if (HotSendGuiDirectoryLooksLikeAssetsIcons(Candidate)) {
            snprintf(Out, Capacity, "%s", Candidate);
            return 0;
        }
    }
    Home = getenv("HOME");
    if (Home != NULL) {
        snprintf(Candidate, sizeof(Candidate), "%s/HotSend/Assets/icons", Home);
        if (HotSendGuiDirectoryLooksLikeAssetsIcons(Candidate)) {
            snprintf(Out, Capacity, "%s", Candidate);
            return 0;
        }
    }
    Length = readlink("/proc/self/exe", Executable, sizeof(Executable) - 1);
    if (Length > 0) {
        char *Slash;

        Executable[Length] = 0;
        Slash = strrchr(Executable, '/');
        if (Slash != NULL) {
            *Slash = 0;
        }
        /* Build/meson/Code → ../../../Assets/icons */
        snprintf(Candidate, sizeof(Candidate), "%s/../../../Assets/icons", Executable);
        if (HotSendGuiDirectoryLooksLikeAssetsIcons(Candidate)) {
            snprintf(Out, Capacity, "%s", Candidate);
            return 0;
        }
        snprintf(Candidate, sizeof(Candidate), "%s/Assets/icons", Executable);
        if (HotSendGuiDirectoryLooksLikeAssetsIcons(Candidate)) {
            snprintf(Out, Capacity, "%s", Candidate);
            return 0;
        }
    }
    if (HotSendGuiDirectoryLooksLikeAssetsIcons("Assets/icons")) {
        snprintf(Out, Capacity, "%s", "Assets/icons");
        return 0;
    }
    return -1;
}

void HotSendGuiApplyWindowIcon(GtkWindow *Window) {
    char IconsDirectory[512];
    GtkIconTheme *Theme;
    GdkDisplay *Display;

    if (Window == NULL) {
        return;
    }
    /* 与 .desktop / gtk_application_new 应用 ID 对齐，便于 Dock 认图标 */
    gtk_window_set_icon_name(Window, "com.blocks.hotsend");
    gtk_window_set_default_icon_name("com.blocks.hotsend");
    if (HotSendGuiResolveIconsDirectory(IconsDirectory, sizeof(IconsDirectory)) != 0) {
        return;
    }
    Display = gtk_widget_get_display(GTK_WIDGET(Window));
    if (Display == NULL) {
        Display = gdk_display_get_default();
    }
    if (Display == NULL) {
        return;
    }
    Theme = gtk_icon_theme_get_for_display(Display);
    if (Theme == NULL) {
        return;
    }
    gtk_icon_theme_add_search_path(Theme, IconsDirectory);
}
