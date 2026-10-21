#include "icons.h"
#include <LittleFS.h>
#include "mbedtls/base64.h"
#include "esp_heap_caps.h"
#include "app_config.h"
#include "profiles.h"
#include "diag_log.h"

#define ICON_DIR "/icons"
#define ICON_MAX_COUNT 100

static bool idOk(const char* id)
{
  size_t n = strlen(id);
  if (!n || n >= 24) return false;
  for (size_t i = 0; i < n; i++) {
    char c = id[i];
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
  }
  return true;
}
static String pathOf(const char* id) { return String(ICON_DIR) + "/" + id + ".bin"; }

// ── Laden fuer die Anzeige ────────────────────────────────────────────────────
bool iconLoad(const char* id, lv_img_dsc_t& dsc, uint8_t** buf)
{
  *buf = nullptr;
  if (!profilesStorageOk() || !idOk(id)) return false;
  File f = LittleFS.open(pathOf(id), "r");
  if (!f) return false;
  uint8_t h[4];
  if (f.read(h, 4) != 4 || h[0] != 'D' || h[1] != 'I' || !h[2] || !h[3] || h[2] > ICON_MAX_SIDE || h[3] > ICON_MAX_SIDE) { f.close(); return false; }
  const size_t n = (size_t)h[2] * h[3] * 3;
  if (f.size() != 4 + n) { f.close(); return false; }
  uint8_t* p = (uint8_t*)heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) { f.close(); return false; }
  if (f.read(p, n) != n) { heap_caps_free(p); f.close(); return false; }
  f.close();
  memset(&dsc, 0, sizeof(dsc));
  dsc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
  dsc.header.w = h[2];
  dsc.header.h = h[3];
  dsc.data_size = n;
  dsc.data = p;
  *buf = p;
  return true;
}

// ── Upload ────────────────────────────────────────────────────────────────────
static uint8_t* s_up = nullptr;
static size_t   s_upLen = 0, s_upCap = 0;
static char     s_upId[24];
static uint8_t  s_upW = 0, s_upH = 0;

static void upFree() { if (s_up) { heap_caps_free(s_up); s_up = nullptr; } s_upLen = s_upCap = 0; }

bool iconBegin(const char* id, int w, int h, const char*& err)
{
  upFree();
  if (!profilesStorageOk()) { err = "kein Speicher"; return false; }
  if (!idOk(id)) { err = "ungueltige id"; return false; }
  if (w < 8 || h < 8 || w > ICON_MAX_SIDE || h > ICON_MAX_SIDE) { err = "Groesse 8..96"; return false; }
  s_upCap = (size_t)w * h * 3;
  s_up = (uint8_t*)heap_caps_malloc(s_upCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!s_up) { s_upCap = 0; err = "Speicher voll"; return false; }
  strlcpy(s_upId, id, sizeof(s_upId));
  s_upW = (uint8_t)w; s_upH = (uint8_t)h; s_upLen = 0;
  return true;
}

bool iconChunk(const char* b64, const char*& err)
{
  if (!s_up) { err = "kein ICONBEGIN"; return false; }
  size_t olen = 0;
  int rc = mbedtls_base64_decode(s_up + s_upLen, s_upCap - s_upLen, &olen, (const unsigned char*)b64, strlen(b64));
  if (rc != 0) { upFree(); err = "base64 oder zu viele Daten"; return false; }
  s_upLen += olen;
  return true;
}

bool iconEnd(const char*& err)
{
  if (!s_up) { err = "kein ICONBEGIN"; return false; }
  if (s_upLen != s_upCap) { upFree(); err = "unvollstaendig"; return false; }
  if (!LittleFS.exists(ICON_DIR)) LittleFS.mkdir(ICON_DIR);
  const uint32_t t0 = millis();
  String path = pathOf(s_upId), tmp = path + ".tmp";
  File f = LittleFS.open(tmp, "w");
  bool ok = false;
  if (f) {
    const uint8_t h[4] = { 'D', 'I', s_upW, s_upH };
    ok = f.write(h, 4) == 4 && f.write(s_up, s_upCap) == s_upCap;
    f.close();
  }
  if (ok) { LittleFS.remove(path); ok = LittleFS.rename(tmp, path); } else LittleFS.remove(tmp);
  logDiag("[Flash] Icon '%s' %ux%u geschrieben: %s, %lu ms", s_upId, s_upW, s_upH, ok ? "ok" : "FEHLER", (unsigned long)(millis() - t0));
  diagFlashWritten();
  upFree();
  if (!ok) err = "Schreiben fehlgeschlagen";
  return ok;
}

// ── Liste, Loeschen, Lesen ────────────────────────────────────────────────────
void iconListJson(String& out)
{
  out = "{\"icons\":[";
  bool first = true;
  File dir = LittleFS.open(ICON_DIR);
  if (dir && dir.isDirectory()) {
    int n = 0;
    for (File f = dir.openNextFile(); f && n < ICON_MAX_COUNT; f = dir.openNextFile()) {
      String name = f.name();
      int sl = name.lastIndexOf('/'); if (sl >= 0) name = name.substring(sl + 1);
      if (f.isDirectory() || !name.endsWith(".bin")) continue;
      uint8_t h[4];
      if (f.read(h, 4) != 4 || h[0] != 'D' || h[1] != 'I') continue;
      name.remove(name.length() - 4);
      if (!idOk(name.c_str())) continue;
      if (!first) out += ",";
      first = false; n++;
      out += "{\"id\":\"" + name + "\",\"w\":" + String(h[2]) + ",\"h\":" + String(h[3]) + "}";
    }
  }
  out += "]}";
}

bool iconDelete(const char* id)
{
  if (!profilesStorageOk() || !idOk(id)) return false;
  return LittleFS.remove(pathOf(id));
}

char* iconMakeLine(const char* id)
{
  if (!profilesStorageOk() || !idOk(id)) return nullptr;
  File f = LittleFS.open(pathOf(id), "r");
  if (!f) return nullptr;
  uint8_t h[4];
  if (f.read(h, 4) != 4 || h[0] != 'D' || h[1] != 'I') { f.close(); return nullptr; }
  const size_t n = (size_t)h[2] * h[3] * 3;
  uint8_t* raw = (uint8_t*)heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!raw) { f.close(); return nullptr; }
  bool ok = f.read(raw, n) == n;
  f.close();
  size_t blen = 0;
  mbedtls_base64_encode(nullptr, 0, &blen, raw, n);
  char* line = ok ? (char*)heap_caps_malloc(blen + 32, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : nullptr;
  if (line) {
    int off = snprintf(line, 32, "ICON %u %u ", h[2], h[3]);
    size_t olen = 0;
    mbedtls_base64_encode((unsigned char*)line + off, blen + 1, &olen, raw, n);
    line[off + olen] = 0;
  }
  heap_caps_free(raw);
  return line;
}
