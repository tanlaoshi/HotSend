/*
 * SerialPorts.c — 枚举 /dev 串口设备 + 波特率下拉
 */
#include "SerialPorts.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const HotSendGuiBaudChoices[] = {
    "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600", NULL};

const char *HotSendGuiDropDownSelectedText(GtkDropDown *DropDown) {
    GtkStringObject *Object;

    if (DropDown == NULL) {
        return NULL;
    }
    Object = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(DropDown));
    if (Object == NULL) {
        return NULL;
    }
    return gtk_string_object_get_string(Object);
}

static int HotSendGuiDeviceNameUseful(const char *Name) {
    return strncmp(Name, "ttyUSB", 6) == 0 || strncmp(Name, "ttyACM", 6) == 0 ||
           strncmp(Name, "ttyS", 4) == 0;
}

/* USB/转接器优先；板载 ttyS* 靠后 */
static int HotSendGuiDeviceRank(const char *Path) {
    const char *Name = Path;

    if (Path == NULL) {
        return 99;
    }
    if (strncmp(Path, "/dev/", 5) == 0) {
        Name = Path + 5;
    }
    if (strncmp(Name, "ttyUSB", 6) == 0) {
        return 0;
    }
    if (strncmp(Name, "ttyACM", 6) == 0) {
        return 1;
    }
    if (strncmp(Name, "ttyS", 4) == 0) {
        return 8;
    }
    return 4;
}

static int HotSendGuiCompareDevicePaths(const void *Left, const void *Right) {
    const char *LeftPath = *(const char *const *)Left;
    const char *RightPath = *(const char *const *)Right;
    int LeftRank = HotSendGuiDeviceRank(LeftPath);
    int RightRank = HotSendGuiDeviceRank(RightPath);

    if (LeftRank != RightRank) {
        return LeftRank - RightRank;
    }
    return strcmp(LeftPath, RightPath);
}

void HotSendGuiSerialRefreshDevices(HotSendGuiWidgets *Widgets,
                                    const char *PreferDevice) {
    DIR *Directory;
    struct dirent *Entry;
    char *Paths[64];
    unsigned Count = 0;
    unsigned Index;
    GtkStringList *List;
    int Selected = 0;
    int FoundPrefer = 0;

    if (Widgets == NULL) {
        return;
    }
    Directory = opendir("/dev");
    if (Directory != NULL) {
        while ((Entry = readdir(Directory)) != NULL && Count < 64u) {
            if (!HotSendGuiDeviceNameUseful(Entry->d_name)) {
                continue;
            }
            Paths[Count] = g_strdup_printf("/dev/%s", Entry->d_name);
            Count++;
        }
        closedir(Directory);
    }
    if (Count > 1u) {
        qsort(Paths, Count, sizeof(Paths[0]), HotSendGuiCompareDevicePaths);
    }
    if (PreferDevice != NULL && PreferDevice[0] != 0) {
        for (Index = 0; Index < Count; Index++) {
            if (strcmp(Paths[Index], PreferDevice) == 0) {
                FoundPrefer = 1;
                Selected = (int)Index;
                break;
            }
        }
        if (!FoundPrefer && Count < 64u) {
            Paths[Count] = g_strdup(PreferDevice);
            Selected = (int)Count;
            Count++;
        }
    }
    if (Count == 0) {
        Paths[0] = g_strdup(PreferDevice != NULL && PreferDevice[0] != 0
                                ? PreferDevice
                                : "/dev/ttyUSB0");
        Count = 1;
        Selected = 0;
    }
    {
        const char **Items = g_new0(const char *, Count + 1);

        for (Index = 0; Index < Count; Index++) {
            Items[Index] = Paths[Index];
        }
        Items[Count] = NULL;
        List = gtk_string_list_new(Items);
        g_free(Items);
    }
    for (Index = 0; Index < Count; Index++) {
        g_free(Paths[Index]);
    }
    gtk_drop_down_set_model(Widgets->DeviceDropDown, G_LIST_MODEL(List));
    gtk_drop_down_set_selected(Widgets->DeviceDropDown, (guint)Selected);
}

void HotSendGuiSerialSelectBaud(HotSendGuiWidgets *Widgets, int BaudRate) {
    char Text[16];
    unsigned Index;

    if (Widgets == NULL || Widgets->BaudDropDown == NULL) {
        return;
    }
    snprintf(Text, sizeof(Text), "%d", BaudRate > 0 ? BaudRate : 115200);
    for (Index = 0; HotSendGuiBaudChoices[Index] != NULL; Index++) {
        if (strcmp(HotSendGuiBaudChoices[Index], Text) == 0) {
            gtk_drop_down_set_selected(Widgets->BaudDropDown, Index);
            return;
        }
    }
    gtk_drop_down_set_selected(Widgets->BaudDropDown, 4); /* 115200 */
}

GtkStringList *HotSendGuiSerialBaudListNew(void) {
    return gtk_string_list_new(HotSendGuiBaudChoices);
}
