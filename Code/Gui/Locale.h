#ifndef HOT_SEND_GUI_LOCALE_H
#define HOT_SEND_GUI_LOCALE_H

#include "Widgets.h"

enum HotSendGuiLanguage {
    HotSendGuiLanguageEnglish = 0,
    HotSendGuiLanguageChinese = 1
};

void HotSendGuiApplyLocale(HotSendGuiWidgets *Widgets, int Language);

#endif
