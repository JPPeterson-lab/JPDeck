#pragma once
// Display bei Inaktivitaet abdunkeln oder ganz ausschalten und beim naechsten Antippen wieder wecken.
// Einstellbar im Editor (Befehl IDLE). Das Antippen, das das Display weckt, wird verschluckt: es loest keine Taste aus.
#include <stdint.h>

// Wird bei jedem Auslesen des Touch-Controllers aufgerufen (LVGL-Task). pressed = rohe Beruehrung.
// Rueckgabe: true, wenn LVGL die Beruehrung sehen darf; false, solange sie zum Wecken gehoert.
bool idleFilterTouch(bool pressed);

void idleTick();            // jede Sekunde (LVGL-Task): dunkel schalten, wenn lange nichts passiert ist
void idleWake();            // Display sofort in den normalen Zustand (z. B. Alarm des Timers)
void idleSuppress(bool on); // true: nicht abdunkeln (Alarm laeuft)
bool idleDimmed();          // Display ist gerade abgedunkelt oder aus
