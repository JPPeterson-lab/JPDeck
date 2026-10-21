#pragma once
// Screen-Navigation (Verdrahtung der PicoPixel-Buttons). Nur aus dem LVGL-Kontext
// bzw. mit gehaltenem lvgl_port_lock() aufrufen.
#include "../ui/ui.h"

void nav_init();                          // Button-Callbacks registrieren (nach ui_init())
void nav_goto(enum ScreensEnum id);       // Screenwechsel ohne Überblendung (spart Rechenzeit)
