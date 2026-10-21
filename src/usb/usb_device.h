#pragma once
// USB-Geraet zum PC ueber esp_tinyusb: Composite aus HID-Tastatur (inkl. Consumer/Media-Tasten) und virtueller
// serieller Schnittstelle (CDC). Das ist reine Hardware-Schicht - sie kennt weder Kacheln noch Profile.
#include <Arduino.h>

// Die COM-Schnittstelle zum PC (nicht "Serial": das ist der UART). Gleiche Handgriffe wie bei einem Arduino-Stream.
struct DeckSerialIdf {
  int  available();
  int  read();
  size_t print(const char* s);
  operator bool() const;                // true, solange der PC den Port geoeffnet hat (DTR)
};
extern DeckSerialIdf DeckSerial;

// Schaltet GPIO19/20 per IO-Expander auf USB und startet HID + CDC. Nach der Display-/Touch-Init aufrufen.
void usbDeviceInit();

// true, solange der PC das Geraet erkannt hat (und nicht im Standby ist)
bool usbHostConnected();

// Taste(n) gleichzeitig druecken und wieder loslassen.
//   Wert < 0x100: HID-Tastencode (physische Taste, z. B. 0x28 = Enter, 0xE0 = Strg links, 0xE3 = Windows)
//   Wert 0x100 | Zeichen: Buchstabe/Ziffer ueber das Tastaturlayout (DECK_KEY_LAYOUT_DE), damit z. B. "z" auf einer
//   deutschen Tastatur die Taste mit dem Z liefert (nicht die an der US-Position, das waere dort das Y).
// Blockiert ca. 30 ms - nicht aus dem LVGL-Task aufrufen, sondern ueber deck_actions.
void usbKeyChord(const uint16_t* keys, uint8_t count);

// Eine Consumer-Taste (Lautstaerke, Play/Pause ...) kurz druecken
void usbConsumerTap(uint16_t usage);

// Text tippen (ASCII, Tastaturlayout des PCs)
void usbTypeText(const char* text);
