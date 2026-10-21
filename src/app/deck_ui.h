#pragma once
// UI-Logik des DeckPad: erzeugt die Kacheln dynamisch aus dem aktiven Profil, verdrahtet Profil-Roller,
// Theme, Helligkeit, Uhr und USB-Status. Kennt LVGL und die Profile, aber kein USB-Protokoll:
// Tastendruecke gehen als deckActionRun() an die Aktionsschicht.
// Alles nur im LVGL-Kontext bzw. mit lvgl_port_lock() aufrufen.

void deckUiInit();                                      // nach ui_init() und nav_init(), Profile muessen geladen sein
void deckUiApplyProfile();                              // Theme, Helligkeit, Titel, Raster des aktiven Profils uebernehmen
void deckUiBootStatus(const char* text, int percent);   // Statuszeile + Balken auf dem Bootscreen

// Fuer COM-Befehle vom PC. Beide nur mit gehaltenem lvgl_port_lock() aufrufen.
void deckUiProfilesChanged();                           // Dateien geaendert: Liste neu lesen, Roller und aktives Profil auffrischen
bool deckUiSelectProfile(const char* id);
void deckUiQuickChanged();                              // Belegung der Tasten P1..P5 geaendert (vom Editor): Tasten auffrischen
void deckUiRedrawAll();                                 // ganzen Bildschirm neu zeichnen (repariert fehlerhaften Bildinhalt im Framebuffer)
void deckUiRebuildGrid();                               // Kacheln neu aufbauen (z. B. nach einem neuen Icon)
void deckUiAutoSwitch(const char* exe);                 // Profil zum Vordergrund-Programm waehlen (nur wenn der Automatik-Schalter an ist)               // Profil per id aktivieren (false = unbekannt)
