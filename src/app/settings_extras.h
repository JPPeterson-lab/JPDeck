#pragma once
// Zusaetze auf dem Einstellungs-Screen, im Code angelegt: Schalter "Automatischer Profilwechsel" und
// "Tasten am PC ausfuehren" samt Helfer-Status.
// (Kann spaeter nach PicoPixel wandern; dann die Objekte dort anlegen und diese Datei durch Verdrahtung ersetzen.)
// Nur im LVGL-Kontext aufrufen, ausser settingsExtrasHelperSeen() und settingsExtrasHelperAlive().
void settingsExtrasInit();         // nach ui_init() und profilesInit()
void settingsExtrasApplyTheme();   // Farben an das aktive Theme anpassen
void settingsExtrasTick();         // jede Sekunde: Statustexte
void settingsExtrasHelperSeen();   // vom CDC-Task: der PC-Helfer hat sich gemeldet (thread-sicher)
bool settingsExtrasHelperAlive();  // true, wenn sich der Helfer in den letzten 10 s gemeldet hat (thread-sicher)
