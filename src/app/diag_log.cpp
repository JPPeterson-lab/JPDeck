#include "diag_log.h"
#include <stdarg.h>
#include "esp_heap_caps.h"
#include "app_config.h"
#include "profiles.h"
#include "../display/lvgl_port.h"
#include "../usb/usb_device.h"

#define LOG_LINES 64
#define LOG_LEN   120

static char     s_buf[LOG_LINES][LOG_LEN];
static uint16_t s_head = 0, s_count = 0;
static SemaphoreHandle_t s_lock = nullptr;

void logDiag(const char* fmt, ...)
{
  if (!s_lock) s_lock = xSemaphoreCreateMutex();             // erster Aufruf kommt aus setup(), noch single-threaded
  char line[LOG_LEN - 12];
  va_list ap; va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  Serial.println(line);                       // UART-Port (Debug-Ausgabe)
  xSemaphoreTake(s_lock, portMAX_DELAY);
  snprintf(s_buf[s_head], LOG_LEN, "%8lu %s", (unsigned long)millis(), line);
  s_head = (s_head + 1) % LOG_LINES;
  if (s_count < LOG_LINES) s_count++;
  xSemaphoreGive(s_lock);
}

static volatile uint32_t s_flashWrites = 0;
void diagFlashWritten() { s_flashWrites = s_flashWrites + 1; }
uint32_t diagFlashCounter() { return s_flashWrites; }

void logDump(void (*emit)(const char*))
{
  if (!s_lock) return;
  xSemaphoreTake(s_lock, portMAX_DELAY);
  uint16_t start = (s_head + LOG_LINES - s_count) % LOG_LINES;
  for (uint16_t i = 0; i < s_count; i++) emit(s_buf[(start + i) % LOG_LINES]);
  xSemaphoreGive(s_lock);
}

void logStatus(char* out, size_t n)
{
  LvglFlushStats f;
  lvglPortPeekFlushStats(f);
  snprintf(out, n,
           "v=%s up=%lus usb=%d sram=%u/min%u/blk%u psram=%u/min%u flush_n=%u avg_us=%u max_us=%u max_px=%u loop_max_us=%u profil=%s fs=%d",
           FIRMWARE_VERSION, (unsigned long)(millis() / 1000), usbHostConnected() ? 1 : 0,
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
           (unsigned)f.count, (unsigned)(f.count ? f.sumUs / f.count : 0), (unsigned)f.maxUs, (unsigned)f.maxPx, (unsigned)f.loopMaxUs,
           profileActive().id, profilesStorageOk() ? 1 : 0);
}
