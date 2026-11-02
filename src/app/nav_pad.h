#pragma once
// 3D-Navigation: eine leere Touch-Flaeche, mit der man in CAD-Programmen die Ansicht dreht, schwenkt und zoomt
// (Idee: Space Mouse, nur per Finger). Mehrfinger-Gesten werden in USB-Mausbewegungen, Mausrad und gehaltene
// Tasten (Shift ...) uebersetzt, so wie das jeweilige Programm sie erwartet (Voreinstellungen fusion, shapr3d, bambu, blender).
// Kein Treiber auf dem PC noetig. UI und Gesten laufen im LVGL-Task; USB geht ueber usbMouseSend() ohne zu blockieren.
#include <stdint.h>

void navPadOpen(const char* presetId);        // Seite oeffnen; leer/unbekannt = zuletzt benutztes Programm
bool navPadPresetValid(const char* presetId); // leer oder eine bekannte Kennung (fuer die Pruefung beim Profil-Import)
