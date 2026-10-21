#pragma once
// Hintergrundbeleuchtung des Waveshare-7B: PWM ueber den IO-Expander (I2C).
// Nur aus dem LVGL-Task aufrufen (der Touch-Treiber nutzt denselben I2C-Bus).
#include <stdint.h>

void backlightInit();               // einschalten (Helligkeit aus gespeichertem Wert folgt spaeter)
void backlightSet(uint8_t percent); // 3..100 %; darunter geht das Display nie ganz aus
uint8_t backlightGet();
