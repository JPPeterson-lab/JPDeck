#include "deck_actions.h"
#include "diag_log.h"
#include <Arduino.h>
#include "esp_timer.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "../usb/usb_device.h"
#include "../usb/usb_cdc.h"
// HID-Verbrauchercodes (Consumer Page), dieselben Werte wie in USBHIDConsumerControl.h
#define CONSUMER_CONTROL_VOLUME_INCREMENT 0x00E9
#define CONSUMER_CONTROL_VOLUME_DECREMENT 0x00EA
#define CONSUMER_CONTROL_MUTE             0x00E2
#define CONSUMER_CONTROL_PLAY_PAUSE       0x00CD
#define CONSUMER_CONTROL_SCAN_NEXT        0x00B5
#define CONSUMER_CONTROL_SCAN_PREVIOUS    0x00B6
#define CONSUMER_CONTROL_STOP             0x00B7
#define CONSUMER_CONTROL_CALCULATOR       0x0192
#define CONSUMER_CONTROL_LOCAL_BROWSER    0x0194
#define CONSUMER_CONTROL_EMAIL_READER     0x018A
#include "profiles.h"
#include "settings_extras.h"
#include "diag_log.h"

// ── Namenstabellen ────────────────────────────────────────────────────────────
struct NameCode { const char* name; uint16_t code; };

static const NameCode MEDIA[] = {
  { "vol_up",     CONSUMER_CONTROL_VOLUME_INCREMENT },
  { "vol_down",   CONSUMER_CONTROL_VOLUME_DECREMENT },
  { "mute",       CONSUMER_CONTROL_MUTE },
  { "play_pause", CONSUMER_CONTROL_PLAY_PAUSE },
  { "next",       CONSUMER_CONTROL_SCAN_NEXT },
  { "prev",       CONSUMER_CONTROL_SCAN_PREVIOUS },
  { "stop",       CONSUMER_CONTROL_STOP },
  { "calculator", CONSUMER_CONTROL_CALCULATOR },
  { "browser",    CONSUMER_CONTROL_LOCAL_BROWSER },
  { "mail",       CONSUMER_CONTROL_EMAIL_READER },
};

// HID-Usage-Codes (Tastaturseite). Modifier liegen bei 0xE0..0xE7.
static const NameCode KEYS[] = {
  { "ctrl", 0xE0 }, { "shift", 0xE1 }, { "alt", 0xE2 }, { "win", 0xE3 }, { "gui", 0xE3 },
  { "rctrl", 0xE4 }, { "rshift", 0xE5 }, { "altgr", 0xE6 }, { "rwin", 0xE7 },
  { "enter", 0x28 }, { "esc", 0x29 }, { "backspace", 0x2A }, { "tab", 0x2B }, { "space", 0x2C },
  { "printscreen", 0x46 }, { "scrolllock", 0x47 }, { "pause", 0x48 }, { "insert", 0x49 },
  { "home", 0x4A }, { "pageup", 0x4B }, { "delete", 0x4C }, { "end", 0x4D }, { "pagedown", 0x4E },
  { "right", 0x4F }, { "left", 0x50 }, { "down", 0x51 }, { "up", 0x52 }, { "capslock", 0x39 },
};

static int lookup(const NameCode* t, size_t n, const char* s, size_t len)
{
  for (size_t i = 0; i < n; i++)
    if (strlen(t[i].name) == len && strncasecmp(t[i].name, s, len) == 0) return (int)t[i].code;
  return -1;
}

// Ein Tastenname -> HID-Code: Namenstabelle, a-z, 0-9, f1-f24
static int keyCode(const char* s, size_t len)
{
  int c = lookup(KEYS, sizeof(KEYS) / sizeof(KEYS[0]), s, len);
  if (c >= 0) return c;
  if (len == 1) {                                  // Buchstaben/Ziffern ueber das Layout (siehe usbKeyChord)
    char ch = (char)tolower((unsigned char)s[0]);
    if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) return 0x100 | ch;
  }
  if ((s[0] == 'f' || s[0] == 'F') && len >= 2 && len <= 3) {
    int n = atoi(s + 1);
    if (n >= 1 && n <= 12) return 0x3A + (n - 1);
    if (n >= 13 && n <= 24) return 0x68 + (n - 13);
  }
  return -1;
}

// "ctrl+shift+m" -> {0xE0, 0xE1, 0x16D}; Rueckgabe: Anzahl oder 0 bei Syntaxfehler (max. 8 Teile)
static uint8_t parseKeys(const char* spec, uint16_t* out, uint8_t maxN)
{
  uint8_t n = 0;
  const char* p = spec;
  while (*p) {
    const char* e = p;
    while (*e && *e != '+') e++;
    size_t len = (size_t)(e - p);
    if (!len || n >= maxN) return 0;
    int c = keyCode(p, len);
    if (c < 0) return 0;
    out[n++] = (uint16_t)c;
    p = *e ? e + 1 : e;
  }
  return n;
}

// ── Namen <-> Typ ─────────────────────────────────────────────────────────────
static const char* const TYPE_NAMES[] = { "none", "media", "keys", "text", "host", "run" };

const char* deckActionTypeName(DeckActionType t) { return t <= ACT_LAST ? TYPE_NAMES[t] : "none"; }

DeckActionType deckActionTypeFromName(const char* name)
{
  if (name) for (uint8_t i = 0; i <= ACT_LAST; i++) if (strcasecmp(name, TYPE_NAMES[i]) == 0) return (DeckActionType)i;
  return ACT_NONE;
}

bool deckActionValid(DeckActionType type, const char* spec)
{
  uint16_t tmp[8];
  switch (type) {
    case ACT_NONE:  return true;
    case ACT_MEDIA: return lookup(MEDIA, sizeof(MEDIA) / sizeof(MEDIA[0]), spec, strlen(spec)) >= 0;
    case ACT_KEYS:  return parseKeys(spec, tmp, sizeof(tmp)) > 0;
    case ACT_TEXT:
    case ACT_HOST:
    case ACT_RUN:   return spec && *spec;
  }
  return false;
}

// ── Task: fuehrt die Aktionen nacheinander aus ───────────────────────────────
struct Job { DeckActionType type; char spec[DECK_SPEC_MAX]; int64_t tEnq; };
static QueueHandle_t s_queue = nullptr;

static void execute(const Job& j)
{
  // Modus "Aktionen am PC": nichts ueber die USB-Tastatur senden, sondern dem PC-Helfer sagen, was er tun soll
  if (profilesHostActions() && j.type != ACT_NONE && j.type != ACT_HOST) {
    if (!settingsExtrasHelperAlive()) { logDiag("[Aktion] %s '%s': PC-Helfer nicht erkannt", deckActionTypeName(j.type), j.spec); return; }
    char line[DECK_SPEC_MAX + 16];
    snprintf(line, sizeof(line), "DO %s %s", deckActionTypeName(j.type), j.spec);
    usbCdcSendLine(line);
    return;
  }
  // Auch ohne gemeldete Verbindung senden: die USB-Bibliothek verwirft Berichte, wenn der PC nicht bereit ist,
  // und ein verpasstes Verbindungs-Ereignis soll nicht alle Tasten stilllegen.
  if (!usbHostConnected() && j.type != ACT_NONE)
    logDiag("[Aktion] %s '%s': PC meldet sich nicht als verbunden", deckActionTypeName(j.type), j.spec);
  switch (j.type) {
    case ACT_MEDIA: {
      int c = lookup(MEDIA, sizeof(MEDIA) / sizeof(MEDIA[0]), j.spec, strlen(j.spec));
      if (c >= 0) usbConsumerTap((uint16_t)c);
      break;
    }
    case ACT_KEYS: {
      uint16_t k[8];
      uint8_t n = parseKeys(j.spec, k, 8);
      if (n) usbKeyChord(k, n);
      break;
    }
    case ACT_TEXT: usbTypeText(j.spec); break;
    case ACT_RUN: {                                  // Win+R oeffnet "Ausfuehren", dort Pfad/Programm/Link + Enter
      const uint16_t winR[2] = { 0xE3, 0x15 };   // Win + R (physische Tasten, auf jedem Layout gleich)
      const uint16_t enter = 0x28;
      usbKeyChord(winR, 2);
      vTaskDelay(pdMS_TO_TICKS(700));                // dem Dialog Zeit zum Oeffnen geben
      usbTypeText(j.spec);
      vTaskDelay(pdMS_TO_TICKS(80));
      usbKeyChord(&enter, 1);
      break;
    }
    case ACT_HOST: {
      char line[DECK_SPEC_MAX + 12];
      snprintf(line, sizeof(line), "ACTION %s", j.spec);
      usbCdcSendLine(line);
      break;
    }
    default: break;
  }
}

static void actionTask(void*)
{
  Job j;
  for (;;) {
    if (xQueueReceive(s_queue, &j, portMAX_DELAY) != pdTRUE) continue;
    const int64_t t0 = esp_timer_get_time();
    execute(j);
    const int64_t t1 = esp_timer_get_time();
    // Zeiten: Warten in der Warteschlange (Druck -> Start) und Ausfuehren (Senden an den PC), in Millisekunden
    logDiag("[Aktion] %s: Warten %ld ms, Senden %ld ms", deckActionTypeName(j.type), (long)((t0 - j.tEnq) / 1000), (long)((t1 - t0) / 1000));
  }
}

void deckActionsInit()
{
  s_queue = xQueueCreate(8, sizeof(Job));
  xTaskCreatePinnedToCore(actionTask, "actions", 4096, nullptr, 2, nullptr, 0);
}

bool deckActionRun(DeckActionType type, const char* spec)
{
  if (type == ACT_NONE || !s_queue) return true;
  Job j;
  j.type = type;
  j.tEnq = esp_timer_get_time();
  strlcpy(j.spec, spec ? spec : "", sizeof(j.spec));
  return xQueueSend(s_queue, &j, 0) == pdTRUE;
}
