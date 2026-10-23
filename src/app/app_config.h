#pragma once
// Zentrale Einstellungen des JPDeck. Alles, was man später mal anpassen will, steht hier.

#define FIRMWARE_VERSION "0.7.0"

// ── USB-Geraet (so meldet sich das Board am Windows-PC an) ─────────────────────
#define DECK_USB_MANUFACTURER "JPPeterson-lab"
#define DECK_USB_PRODUCT      "JPDeck"
// Kennung: Espressif-VID mit eigener PID. 0x1001 gehoert der USB-JTAG/Seriell-Schnittstelle der Chips; deren Windows-Treiber
// (WinUSB, z. B. durch die IDF/OpenOCD-Einrichtung) bindet sich an USB\VID_303A&PID_1001&MI_02 und verdraengt dort die Tastatur.
// PID provisorisch: fuer eine Veroeffentlichung eine eigene beantragen (Espressif vergibt sie kostenlos, oder pid.codes mit VID 0x1209).
#define DECK_USB_VID 0x303A
#define DECK_USB_PID 0x4DC0
// IO5 des IO-Expanders schaltet die Pins GPIO19/20 zwischen USB und CAN um.
// Laut Waveshare-Header: 0 = USB, 1 = CAN. Falls der PC das Board nicht sieht: Wert tauschen.
#define DECK_USB_SELECT_LEVEL 0

// Tastaturlayout des Windows-PCs (gilt fuer Text und Buchstaben/Ziffern in Tastenkombinationen)
#define DECK_KEY_LAYOUT_DE 1            // 1 = deutsches Layout, 0 = US-Layout

// ── Kachelraster ───────────────────────────────────────────────────────────────
#define DECK_DEFAULT_COLS 5     // Standardraster (wie im PicoPixel-Entwurf: 5 x 3 = 15 Tasten)
#define DECK_DEFAULT_ROWS 3
#define DECK_MAX_COLS     6     // Obergrenze pro Profil
#define DECK_MAX_ROWS     4
#define DECK_MAX_TILES    (DECK_MAX_COLS * DECK_MAX_ROWS)
#define DECK_GRID_GAP     16    // Abstand zwischen den Kacheln in Pixeln

// ── Profile im Flash (LittleFS auf der Partition "storage") ───────────────────
#define DECK_MAX_PROFILES   16
#define DECK_STORAGE_LABEL  "storage"
#define DECK_PROFILE_DIR    "/profiles"
#define DECK_CONFIG_FILE    "/config.json"

// ── Helligkeit ─────────────────────────────────────────────────────────────────
#define DECK_BRIGHTNESS_MIN 3
#define DECK_BRIGHTNESS_DEFAULT 80
