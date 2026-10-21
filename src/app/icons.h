#pragma once
// Kachel-Icons: kleine Bilder (RGB565 + Alpha, bis 96 x 96 px) im Flash unter /icons/<id>.bin.
// Der PC-Editor wandelt Symbole/Bilder um und schickt sie per COM-Port; mehrere Profile koennen dasselbe Icon nutzen.
// Dateiformat: 'D','I', Breite, Hoehe, danach Breite*Hoehe*3 Byte (Farbe low, Farbe high, Alpha) = LVGL "TRUE_COLOR_ALPHA".
#include <Arduino.h>
#include <lvgl.h>

#define ICON_MAX_SIDE 96

// Fuer die UI (LVGL-Task): Icon in den PSRAM laden. *buf gehoert dem Aufrufer (heap_caps_free), dsc zeigt darauf.
bool iconLoad(const char* id, lv_img_dsc_t& dsc, uint8_t** buf);

// Fuer die COM-Befehle (CDC-Task): Upload in Stuecken
bool iconBegin(const char* id, int w, int h, const char*& err);
bool iconChunk(const char* base64, const char*& err);
bool iconEnd(const char*& err);
void iconListJson(String& out);                       // {"icons":[{"id":"..","w":48,"h":48},...]}
bool iconDelete(const char* id);
char* iconMakeLine(const char* id);                   // "ICON <w> <h> <base64>" (heap_caps_malloc, vom Aufrufer freigeben) oder nullptr
