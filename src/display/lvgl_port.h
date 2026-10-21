#pragma once
// LVGL-Anbindung für das Waveshare ESP32-S3-Touch-LCD-7B (1024x600, RGB, GT911)
// Rendering: LVGL zeichnet Teilbereiche in einen SRAM-Puffer und kopiert sie per
// esp_lcd_panel_draw_bitmap in den (einzigen) PSRAM-Framebuffer. Kein Full-Refresh.
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "../hardware/touch.h"
#include "lvgl.h"
#include "../hardware/rgb_lcd_port.h"
#include "../hardware/gt911.h"

#define LVGL_PORT_H_RES             (EXAMPLE_LCD_H_RES)
#define LVGL_PORT_V_RES             (EXAMPLE_LCD_V_RES)

// Zeichenpuffer für das Teilrendering. Der LVGL-Speicherpool liegt im PSRAM (conf/lv_conf.h), damit intern
// genug Platz für WLAN und TLS bleibt (TLS braucht ca. 45 KB intern am Stück). Der Zeichenpuffer liegt
// wieder im schnellen internen SRAM: 20 Zeilen = 40 KB. Bei Speichermangel Zeilen verkleinern oder
// LVGL_PORT_BUF_IN_PSRAM 1 mit z.B. 60 Zeilen.
#define LVGL_PORT_BUF_LINES         (20)
#define LVGL_PORT_BUF_IN_PSRAM      (0)
// Aktualisierungsflächen werden in x auf Vielfache von 32 Pixeln (= 64 Byte, eine Cache-Zeile) erweitert,
// damit das Kopieren in den PSRAM-Framebuffer nie mitten in einer Cache-Zeile beginnt oder endet.
#define LVGL_PORT_ALIGN_PX          (32)
// Jeder Streifen wird in Stücken von höchstens so vielen Zeilen in den Framebuffer kopiert. Ein großer Kopiervorgang
// samt Cache-Rückschreiben blockiert den PSRAM so lange, dass das Display (Bounce-Buffer) leer läuft: Streifen am
// linken Rand, nach oben verschobenes Bild. Kleine Stücke lassen dem Display-Interrupt zwischendurch Luft.
#define LVGL_PORT_FLUSH_CHUNK_LINES (4)
#define LVGL_PORT_PERF_LOG          (0)           // 1 = Renderzeiten pro Bildaufbau im Seriellen Monitor ausgeben
#define LVGL_PORT_TASK_MAX_DELAY_MS (500)
#define LVGL_PORT_TASK_MIN_DELAY_MS (10)
#define LVGL_PORT_TASK_STACK_SIZE   (10 * 1024)
#define LVGL_PORT_TASK_PRIORITY     (2)
#define LVGL_PORT_TASK_CORE         (1)           // WLAN/HTTP laufen auf Core 0

esp_err_t lvgl_port_init(esp_lcd_panel_handle_t lcd_handle, esp_lcd_touch_handle_t tp_handle);
void lvglPortRestartPanel(void);         // RGB-Panel-DMA neu starten (rettet das Bild, wenn LCD-Controller und DMA aus dem Takt geraten sind)
bool lvgl_port_lock(int timeout_ms);   // -1 = unbegrenzt warten
void lvgl_port_unlock(void);

// Statistik der Flush-Aufrufe (Kopieren in den PSRAM-Framebuffer) seit dem letzten Zuruecksetzen.
// Lange Flushes sind der Verdacht bei Streifen und zerrissenen Zeilen (PSRAM-Bandbreite).
struct LvglFlushStats { uint32_t count, sumUs, maxUs, maxPx, loopMaxUs; };   // loopMaxUs: laengste Runde der LVGL-Schleife (Zeichnen + Eingaben)
void lvglPortPeekFlushStats(LvglFlushStats& out);   // lesen
void lvglPortTakeFlushStats(LvglFlushStats& out);   // lesen und zuruecksetzen (nur aus dem LVGL-Task)

// Ausreisser: ein einzelner Flush, der laenger als LVGL_PORT_SLOW_FLUSH_US dauert (Verdacht: PSRAM oder Cache blockiert)
#define LVGL_PORT_SLOW_FLUSH_US 20000
struct LvglSlowFlush { uint32_t n, maxUs, px; int x, y, w, h; };
bool lvglPortTakeSlowFlush(LvglSlowFlush& out);     // true, wenn seit dem letzten Aufruf einer vorkam
