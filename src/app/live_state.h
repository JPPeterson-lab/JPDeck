#pragma once
// Live-Werte vom PC (vom Windows-Helfer per COM-Port gemeldet) fuer Kacheln mit Anzeige:
// vol (Lautstaerke in %), mute (Wiedergabe stumm), mic (Mikrofon stumm), cpu (Last in %), ram (Last in %).
#include <stddef.h>

void liveUpdate(const char* args);                 // "vol=45 mute=0 mic=1 cpu=23 ram=61" (aus dem Befehl LIVE)
bool liveAlive();                                  // Werte sind aktuell (in den letzten 5 s gemeldet)
bool liveMetricValid(const char* metric);          // "" oder eine der Kennungen oben
// Text fuer die Kachel; false, wenn der Wert fehlt ("--"). alert: Kachel soll warnen (stumm, Last >= 90 %)
bool liveFormat(const char* metric, char* text, size_t n, bool& alert);
