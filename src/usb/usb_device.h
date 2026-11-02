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

// Maus und gehaltene Modifier fuer die 3D-Navigation. Nicht blockierend: die Berichte laufen ueber eine Warteschlange
// in einen eigenen Task und duerfen darum aus dem LVGL-Task gesendet werden.
//   buttons: Bit0 links, Bit1 rechts, Bit2 mitte (gilt bis zum naechsten Aufruf, 0 = alle loslassen)
//   dx, dy:  relative Bewegung; wheel: Mausrad-Rasten (positiv = von sich weg)
//   hwheel: waagerechtes Rad (positiv = nach rechts)
void usbMouseSend(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel, int8_t hwheel = 0);
//   mods: Bit0 Strg, Bit1 Shift, Bit2 Alt, Bit3 Windows; wird gehalten, bis ein neuer Wert (z. B. 0) kommt
void usbKeyboardMods(uint8_t mods);
//   Taste kurz antippen (HID-Tastencode, optional mit Modifier), nicht blockierend; fuer Schritt-Gesten (Zeitleiste, Rollen)
void usbKeyTapAsync(uint8_t code, uint8_t mods = 0);
