#include "live_state.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

static volatile int s_vol = -1, s_mute = -1, s_mic = -1, s_cpu = -1, s_ram = -1;
static volatile uint32_t s_ms = 0;
#define LIVE_TIMEOUT_MS 5000

bool liveAlive() { const uint32_t t = s_ms; return t && (uint32_t)(millis() - t) < LIVE_TIMEOUT_MS; }

void liveUpdate(const char* args)
{
  int vol = -1, mute = -1, mic = -1, cpu = -1, ram = -1;
  const char* p = args;
  while (p && *p) {
    while (*p == ' ') p++;
    const char* eq = strchr(p, '=');
    if (!eq) break;
    const int v = atoi(eq + 1);
    const size_t kl = (size_t)(eq - p);
    if (kl == 3 && !strncmp(p, "vol", 3)) vol = v;
    else if (kl == 4 && !strncmp(p, "mute", 4)) mute = v;
    else if (kl == 3 && !strncmp(p, "mic", 3)) mic = v;
    else if (kl == 3 && !strncmp(p, "cpu", 3)) cpu = v;
    else if (kl == 3 && !strncmp(p, "ram", 3)) ram = v;
    p = strchr(eq, ' ');
  }
  s_vol = vol; s_mute = mute; s_mic = mic; s_cpu = cpu; s_ram = ram;
  s_ms = millis() ? millis() : 1;
}

bool liveMetricValid(const char* m)
{
  if (!m || !*m) return true;
  return !strcmp(m, "vol") || !strcmp(m, "mute") || !strcmp(m, "mic") || !strcmp(m, "cpu") || !strcmp(m, "ram");
}

bool liveFormat(const char* m, char* text, size_t n, bool& alert)
{
  alert = false;
  if (!liveAlive() || !m) { snprintf(text, n, "--"); return false; }
  const int vol = s_vol, mute = s_mute, mic = s_mic, cpu = s_cpu, ram = s_ram;
  if (!strcmp(m, "vol"))  { if (vol < 0) goto none; alert = mute == 1; if (alert) snprintf(text, n, "Stumm"); else snprintf(text, n, "%d %%", vol); return true; }
  if (!strcmp(m, "mute")) { if (mute < 0) goto none; alert = mute == 1; snprintf(text, n, alert ? "Ton aus" : "Ton an"); return true; }
  if (!strcmp(m, "mic"))  { if (mic < 0) goto none; alert = mic == 1; snprintf(text, n, alert ? "Mikro aus" : "Mikro an"); return true; }
  if (!strcmp(m, "cpu"))  { if (cpu < 0) goto none; alert = cpu >= 90; snprintf(text, n, "%d %%", cpu); return true; }
  if (!strcmp(m, "ram"))  { if (ram < 0) goto none; alert = ram >= 90; snprintf(text, n, "%d %%", ram); return true; }
none:
  snprintf(text, n, "--");
  return false;
}
