#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <Arduino.h>
#include "lvgl_port.h"
#include "src/ui/screens.h"
#include <stdint.h>

static const char *TAG = "lv_port";
static SemaphoreHandle_t lvgl_mux;

#if LVGL_PORT_PERF_LOG
static uint32_t flush_us_sum = 0;   // Zeit, die das Kopieren in den Framebuffer gekostet hat
static void monitor_cb(lv_disp_drv_t *, uint32_t ms, uint32_t px)
{
    if (px > 20000) {   // nur grosse Aktualisierungen (Screenwechsel), keine kleinen Label-Updates
        // Die ersten 4 Eintraege von objects sind die Screens, in der Reihenfolge von ScreensEnum
        static const char *names[] = { "setting", "lager", "deck", "boot" };
        const char *name = "code-screen";   // Rechner und Timer entstehen im Code und stehen nicht in objects
        for (int i = 0; i < 4; i++)
            if (((lv_obj_t **)&objects)[i] == lv_scr_act()) { name = names[i]; break; }
        Serial.printf("[LVGL] %-13s Bildaufbau %u ms gesamt, davon %u ms Kopieren, %u px\n",
                      name, (unsigned)ms, (unsigned)(flush_us_sum / 1000), (unsigned)px);
    }
    flush_us_sum = 0;
}
#endif

static esp_lcd_panel_handle_t s_panel = nullptr;

void lvglPortRestartPanel(void)
{
    // Setzt nur ein Flag; der Treiber startet beim naechsten VSYNC neu (siehe esp_lcd_panel_rgb.h)
    if (s_panel) esp_lcd_rgb_panel_restart(s_panel);
}

static volatile LvglFlushStats s_stats = {0, 0, 0, 0, 0};
static volatile uint32_t s_slowN = 0, s_slowUs = 0, s_slowPx = 0;
static volatile int s_slowX = 0, s_slowY = 0, s_slowW = 0, s_slowH = 0;

bool lvglPortTakeSlowFlush(LvglSlowFlush& out)
{
    if (!s_slowN) return false;
    out.n = s_slowN; out.maxUs = s_slowUs; out.px = s_slowPx;
    out.x = s_slowX; out.y = s_slowY; out.w = s_slowW; out.h = s_slowH;
    s_slowN = s_slowUs = s_slowPx = 0;
    return true;
}

void lvglPortPeekFlushStats(LvglFlushStats& out)
{
    out.count = s_stats.count; out.sumUs = s_stats.sumUs; out.maxUs = s_stats.maxUs; out.maxPx = s_stats.maxPx; out.loopMaxUs = s_stats.loopMaxUs;
}

void lvglPortTakeFlushStats(LvglFlushStats& out)
{
    lvglPortPeekFlushStats(out);
    s_stats.count = s_stats.sumUs = s_stats.maxUs = s_stats.maxPx = s_stats.loopMaxUs = 0;
}

static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)drv->user_data;
    int64_t t0 = esp_timer_get_time();
    // Kopiert den Bereich synchron in den Framebuffer (inkl. Cache-Rückschreiben), in kleinen Stücken
    const int w = area->x2 - area->x1 + 1;
    for (int y = area->y1; y <= area->y2; y += LVGL_PORT_FLUSH_CHUNK_LINES) {
        int yEnd = min(y + LVGL_PORT_FLUSH_CHUNK_LINES, (int)area->y2 + 1);
        esp_lcd_panel_draw_bitmap(panel, area->x1, y, area->x2 + 1, yEnd, color_map + (size_t)(y - area->y1) * w);
    }
    uint32_t us = (uint32_t)(esp_timer_get_time() - t0);
    uint32_t px = (uint32_t)w * (uint32_t)(area->y2 - area->y1 + 1);
    s_stats.count = s_stats.count + 1;
    s_stats.sumUs = s_stats.sumUs + us;
    if (us > s_stats.maxUs) s_stats.maxUs = us;
    if (px > s_stats.maxPx) s_stats.maxPx = px;
    if (us > LVGL_PORT_SLOW_FLUSH_US) {
        s_slowN = s_slowN + 1;
        if (us > s_slowUs) { s_slowUs = us; s_slowPx = px; s_slowX = area->x1; s_slowY = area->y1; s_slowW = w; s_slowH = area->y2 - area->y1 + 1; }
    }
#if LVGL_PORT_PERF_LOG
    flush_us_sum += us;
#endif
    lv_disp_flush_ready(drv);
}

static void rounder_cb(lv_disp_drv_t *, lv_area_t *area)
{
    area->x1 &= ~(LVGL_PORT_ALIGN_PX - 1);
    area->x2 |= (LVGL_PORT_ALIGN_PX - 1);
    if (area->x2 >= LVGL_PORT_H_RES) area->x2 = LVGL_PORT_H_RES - 1;
}

static void touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)drv->user_data;
    uint16_t x, y;
    uint8_t cnt = 0;
    esp_lcd_touch_read_data(tp);
    if (esp_lcd_touch_get_coordinates(tp, &x, &y, NULL, &cnt, 1) && cnt > 0) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static lv_disp_t *display_init(esp_lcd_panel_handle_t panel)
{
    s_panel = panel;
    static lv_disp_draw_buf_t draw_buf;
    static lv_disp_drv_t disp_drv;

    const size_t px = (size_t)LVGL_PORT_H_RES * LVGL_PORT_BUF_LINES;
#if LVGL_PORT_BUF_IN_PSRAM
    void *buf = heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
#else
    void *buf = heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf) {
        ESP_LOGW(TAG, "Kein internes SRAM fuer Zeichenpuffer, weiche auf PSRAM aus (langsamer)");
        buf = heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    }
#endif
    assert(buf);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, px);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res   = LVGL_PORT_H_RES;
    disp_drv.ver_res   = LVGL_PORT_V_RES;
    disp_drv.flush_cb  = flush_cb;
    disp_drv.rounder_cb = rounder_cb;
    disp_drv.draw_buf  = &draw_buf;
    disp_drv.user_data = panel;
#if LVGL_PORT_PERF_LOG
    disp_drv.monitor_cb = monitor_cb;
#endif
    return lv_disp_drv_register(&disp_drv);
}

static void lvgl_port_task(void *)
{
    uint32_t delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_port_lock(-1)) {
            const int64_t tl0 = esp_timer_get_time();
            delay_ms = lv_timer_handler();
            const uint32_t tl = (uint32_t)(esp_timer_get_time() - tl0);
            if (tl > s_stats.loopMaxUs) s_stats.loopMaxUs = tl;
            lvgl_port_unlock();
        }
        if (delay_ms > LVGL_PORT_TASK_MAX_DELAY_MS) delay_ms = LVGL_PORT_TASK_MAX_DELAY_MS;
        else if (delay_ms < LVGL_PORT_TASK_MIN_DELAY_MS) delay_ms = LVGL_PORT_TASK_MIN_DELAY_MS;
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

esp_err_t lvgl_port_init(esp_lcd_panel_handle_t lcd_handle, esp_lcd_touch_handle_t tp_handle)
{
    lv_init();   // Tick kommt aus lv_conf.h (LV_TICK_CUSTOM -> millis())

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);

    assert(display_init(lcd_handle));

    if (tp_handle) {
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type      = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb   = touchpad_read;
        indev_drv.user_data = tp_handle;
        assert(lv_indev_drv_register(&indev_drv));
    }

    if (xTaskCreatePinnedToCore(lvgl_port_task, "lvgl", LVGL_PORT_TASK_STACK_SIZE, NULL,
                                LVGL_PORT_TASK_PRIORITY, NULL, LVGL_PORT_TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "LVGL-Task konnte nicht gestartet werden");
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool lvgl_port_lock(int timeout_ms)
{
    const TickType_t t = (timeout_ms < 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, t) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    xSemaphoreGiveRecursive(lvgl_mux);
}
