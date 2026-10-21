# Credits & Drittanbieter-Lizenzen

Dieses Projekt verwendet Ressourcen Dritter. Die jeweiligen Lizenzbedingungen
sind nachfolgend aufgeführt.

---

## Treiber

**Quelle:** Waveshare-Beispielcode für das ESP32-S3-Touch-LCD-7B (RGB-Panel, I²C, IO-Expander) sowie der GT911-Treiber auf Basis von Espressif `esp_lcd_touch`
(Apache License 2.0, © Espressif Systems (Shanghai) CO LTD). Siehe Dateikopf in `src/hardware/`.

---

## Bibliotheken

| Bibliothek | Lizenz |
|---|---|
| [LVGL](https://lvgl.io) 8.4 (inkl. lodepng) | MIT / zlib |
| [ArduinoJson](https://arduinojson.org) 7 | MIT |
| Arduino-ESP32 / ESP-IDF | LGPL 2.1 / Apache 2.0 |

## Icons der Oberfläche

**Quelle:** [Bootstrap Icons](https://icons.getbootstrap.com/) ([GitHub](https://github.com/twbs/icons)), seit v0.6.1

**Lizenz:** MIT, Copyright (c) 2019-2024 The Bootstrap Authors

Verwendet werden die Symbole Haus, Pfeil links/rechts, Taschenrechner, Wecker, Sternchen und die Ziffern 1 bis 5 im Quadrat (`house`, `arrow-left`, `arrow-right`, `calculator`, `alarm`, `asterisk`, `1-square` bis `5-square`). Sie sind als Bilddaten in die Firmware eingebettet (`src/ui/images/`).

Bis v0.6.0 stammten die Symbole aus den [Flat Color Icons](https://github.com/icons8/flat-color-icons) von Icons8 (MIT / Good Boy License); sie werden nicht mehr verwendet.

---

## Tastaturlayouts

`src/usb/keyboard_layouts.h`: ASCII→HID-Tabellen (Deutsch, US) aus der USB-Bibliothek von Arduino-ESP32 (`USBHIDKeyboard`, LGPL-2.1).

## Oberfläche & Schrift

- UI-Design und Code-Export: [PicoPixel](https://picopixel.io)
- Schrift: Montserrat (SIL Open Font License 1.1)
