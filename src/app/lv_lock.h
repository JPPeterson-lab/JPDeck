#pragma once
// RAII-Sperre fuer LVGL: Code ausserhalb des LVGL-Tasks (z. B. CDC- oder Aktions-Task) muss
// vor jedem lv_*-Aufruf sperren. Im LVGL-Task selbst (Event-Callbacks, lv_timer) nicht noetig.
#include "../display/lvgl_port.h"

struct LvLock {
  LvLock()  { lvgl_port_lock(-1); }
  ~LvLock() { lvgl_port_unlock(); }
};
