#pragma once
// Schriften, die der Code selbst braucht (Rechner- und Timer-Anzeige) und die PicoPixel nicht exportiert,
// weil sie in der UI nicht vorkommen. Liegen bewusst ausserhalb von src/ui/, damit ein neuer Export sie nicht loescht.
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif
extern const lv_font_t font_montserrat_62;
#ifdef __cplusplus
}
#endif
