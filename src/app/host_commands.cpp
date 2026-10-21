#include "host_commands.h"
#include <Arduino.h>
#include <sys/time.h>
#include "app_config.h"
#include "profiles.h"
#include "deck_ui.h"
#include "diag_log.h"
#include "lv_lock.h"
#include "icons.h"
#include "settings_extras.h"
#include "esp_heap_caps.h"
#include "../display/lvgl_port.h"
#include "../usb/usb_cdc.h"

// Antworten: Einzeiler. Erfolg "OK", Fehler "ERR <Grund>", LIST/GET/STATUS liefern direkt ihre Daten.
// Wichtig: Datei-Zugriffe (profiles.*) laufen hier im CDC-Task, alles an LVGL erst danach unter LvLock.
// Nie umgekehrt (erst LvLock, dann Profil-Mutex) - sonst Deadlock mit dem LVGL-Task.

static void err(const char* why) { char b[96]; snprintf(b, sizeof(b), "ERR %s", why); usbCdcSendLine(b); }

static void cmdVer(const char*)  { usbCdcSendLine("DECKPAD " FIRMWARE_VERSION); }

static void cmdList(const char*) { String s; profileListJson(s); usbCdcSendLine(s.c_str()); }

static void cmdGet(const char* a)
{
  String s;
  if (!profileReadRaw(a, s)) { err("unbekannt"); return; }
  usbCdcSendLine(s.c_str());
}

// PUT <id> <json>   (legt an oder ueberschreibt)
static void cmdPut(const char* a)
{
  char id[32];
  const char* sp = strchr(a, ' ');
  if (!sp || (size_t)(sp - a) >= sizeof(id)) { err("Format: PUT <id> <json>"); return; }
  memcpy(id, a, sp - a); id[sp - a] = 0;
  const char* why = "";
  if (!profileImportJson(id, sp + 1, why)) { err(why); return; }
  { LvLock lock; deckUiProfilesChanged(); }
  logDiag("[PC] Profil '%s' empfangen", id);
  usbCdcSendLine("OK");
}

static void cmdDel(const char* a)
{
  if (!profileDelete(a)) { err("nicht geloescht (unbekannt oder letztes Profil)"); return; }
  { LvLock lock; deckUiProfilesChanged(); }
  logDiag("[PC] Profil '%s' geloescht", a);
  usbCdcSendLine("OK");
}

static void cmdUse(const char* a)
{
  bool ok;
  { LvLock lock; ok = deckUiSelectProfile(a); }
  if (ok) usbCdcSendLine("OK"); else err("unbekannt");
}

// TIME <Unix-Sekunden UTC>   (die Zeitzone CET/CEST ist im Geraet hinterlegt)
static void cmdTime(const char* a)
{
  long long t = atoll(a);
  if (t < 1700000000LL) { err("Zeit ungueltig"); return; }
  struct timeval tv = { (time_t)t, 0 };
  settimeofday(&tv, nullptr);
  logDiag("[PC] Uhrzeit gesetzt");
  usbCdcSendLine("OK");
}

// QUICK <id1> ... <id5>   Belegung der Tasten P1..P5 ("-" = frei); ohne Angabe: aktuelle Belegung zeigen (wie LIST)
static void cmdQuick(const char* a)
{
  char id[DECK_QUICK_COUNT][24];
  int n = a && *a ? sscanf(a, "%23s %23s %23s %23s %23s", id[0], id[1], id[2], id[3], id[4]) : 0;
  if (n != DECK_QUICK_COUNT) { err("Format: QUICK <id1> <id2> <id3> <id4> <id5> (- = frei)"); return; }
  for (int i = 0; i < DECK_QUICK_COUNT; i++) profilesSetQuick(i, strcmp(id[i], "-") == 0 ? "" : id[i]);
  { LvLock lock; deckUiQuickChanged(); }
  logDiag("[PC] Schnellwahl P1..P5 gesetzt");
  usbCdcSendLine("OK");
}

// ACTIVE <programm.exe>   vom PC-Helfer: Vordergrund-Programm gemeldet (wird alle paar Sekunden wiederholt)
static void cmdActive(const char* a)
{
  settingsExtrasHelperSeen();
  if (!profilesAutoSwitch()) { usbCdcSendLine("OK aus"); return; }
  { LvLock lock; deckUiAutoSwitch(a); }
  usbCdcSendLine("OK");
}

// ICONBEGIN <id> <w> <h>, ICONDATA <base64> (mehrfach), ICONEND
static void cmdIconBegin(const char* a)
{
  char id[32]; int w = 0, h = 0;
  if (sscanf(a, "%31s %d %d", id, &w, &h) != 3) { err("Format: ICONBEGIN <id> <w> <h>"); return; }
  const char* why = "";
  if (!iconBegin(id, w, h, why)) { err(why); return; }
  usbCdcSendLine("OK");
}
static void cmdIconData(const char* a)
{
  const char* why = "";
  if (!iconChunk(a, why)) { err(why); return; }
  usbCdcSendLine("OK");
}
static void cmdIconEnd(const char*)
{
  const char* why = "";
  if (!iconEnd(why)) { err(why); return; }
  { LvLock lock; deckUiRebuildGrid(); }                      // falls das aktive Profil dieses Icon nutzt
  usbCdcSendLine("OK");
}
static void cmdIconList(const char*) { String s; iconListJson(s); usbCdcSendLine(s.c_str()); }
static void cmdIconDel(const char* a)
{
  if (!iconDelete(a)) { err("unbekannt"); return; }
  { LvLock lock; deckUiRebuildGrid(); }
  usbCdcSendLine("OK");
}
static void cmdIconGet(const char* a)
{
  char* line = iconMakeLine(a);
  if (!line) { err("unbekannt"); return; }
  usbCdcSendLine(line);
  heap_caps_free(line);
}

static void cmdRedraw(const char*) { { LvLock lock; deckUiRedrawAll(); } logDiag("[PC] Vollbild neu zeichnen angefordert"); usbCdcSendLine("OK"); }

static void cmdPanel(const char*) { { LvLock lock; lvglPortRestartPanel(); } logDiag("[PC] Panel-Neustart angefordert"); usbCdcSendLine("OK"); }

static void cmdStatus(const char*) { char b[320]; logStatus(b, sizeof(b)); usbCdcSendLine(b); }
static void emitLine(const char* l) { usbCdcSendLine(l); }
static void cmdLog(const char*) { usbCdcSendLine("--- LOG Anfang ---"); logDump(emitLine); usbCdcSendLine("--- LOG Ende ---"); }

void hostCommandsInit()
{
  usbCdcRegister("VER",    cmdVer);
  usbCdcRegister("LIST",   cmdList);
  usbCdcRegister("GET",    cmdGet);
  usbCdcRegister("PUT",    cmdPut);
  usbCdcRegister("DEL",    cmdDel);
  usbCdcRegister("USE",    cmdUse);
  usbCdcRegister("QUICK",  cmdQuick);
  usbCdcRegister("TIME",   cmdTime);
  usbCdcRegister("ACTIVE",    cmdActive);
  usbCdcRegister("ICONBEGIN", cmdIconBegin);
  usbCdcRegister("ICONDATA",  cmdIconData);
  usbCdcRegister("ICONEND",   cmdIconEnd);
  usbCdcRegister("ICONLIST",  cmdIconList);
  usbCdcRegister("ICONDEL",   cmdIconDel);
  usbCdcRegister("ICONGET",   cmdIconGet);
  usbCdcRegister("REDRAW", cmdRedraw);
  usbCdcRegister("PANEL",  cmdPanel);
  usbCdcRegister("STATUS", cmdStatus);
  usbCdcRegister("LOG",    cmdLog);
}
