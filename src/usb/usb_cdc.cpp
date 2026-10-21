#include "usb_cdc.h"
#include "usb_device.h"
#include <string.h>
#include <strings.h>

// ── Befehlstabelle ────────────────────────────────────────────────────────────
struct Cmd { const char* name; UsbCdcHandler fn; };
#define CDC_MAX_CMDS 24
static Cmd    s_cmds[CDC_MAX_CMDS];
static size_t s_cmdN = 0;

bool usbCdcRegister(const char* name, UsbCdcHandler fn)
{
  if (s_cmdN >= CDC_MAX_CMDS) return false;
  s_cmds[s_cmdN++] = { name, fn };
  return true;
}

static void cmdPing(const char*) { usbCdcSendLine("PONG"); }

// ── Zeilenleser ───────────────────────────────────────────────────────────────
#define CDC_LINE_MAX 6144    // ein Profil als JSON-Zeile (24 Kacheln) braucht ca. 4 KB

static void handleLine(char* line)
{
  while (*line == ' ' || *line == '\t') line++;
  size_t n = strlen(line);
  while (n && (line[n - 1] == ' ' || line[n - 1] == '\t')) line[--n] = 0;
  if (!n) return;

  char* args = line;
  while (*args && *args != ' ') args++;
  if (*args) { *args++ = 0; while (*args == ' ') args++; }

  for (size_t i = 0; i < s_cmdN; i++) {
    if (strcasecmp(line, s_cmds[i].name) == 0) { s_cmds[i].fn(args); return; }   // Gross-/Kleinschreibung egal
  }
  // Unbekannte Eingaben werden bewusst nicht beantwortet.
}

static void cdcTask(void*)
{
  static char buf[CDC_LINE_MAX];
  size_t len = 0;
  for (;;) {
    while (DeckSerial.available()) {
      int c = DeckSerial.read();
      if (c == '\n' || c == '\r') {
        if (len) { buf[len] = 0; handleLine(buf); len = 0; }
      } else if (len < CDC_LINE_MAX - 1) {
        buf[len++] = (char)c;
      } else {
        len = 0;                    // Zeile zu lang: verwerfen
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void usbCdcSendLine(const char* line)
{
  if (!DeckSerial) return;          // kein Terminal geoeffnet (DTR aus): nichts senden, nicht blockieren
  DeckSerial.print(line);
  DeckSerial.print("\r\n");
}

void usbCdcInit()
{
  usbCdcRegister("PING", cmdPing);
  xTaskCreatePinnedToCore(cdcTask, "cdc", 8192, nullptr, 1, nullptr, 0);   // Core 0, LVGL laeuft auf Core 1
}
