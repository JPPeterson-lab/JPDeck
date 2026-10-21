#pragma once
// Profile: pro Programm eigene Tasten (Kacheln) und Rastergroesse. Design (hell/dunkel) und Helligkeit gelten fuer das ganze Geraet.
// Jedes Profil ist eine JSON-Datei in /profiles/<id>.json auf der Partition "storage" (LittleFS).
// Diese Schicht kennt weder LVGL noch USB - nur Daten und Dateien.
#include <Arduino.h>
#include "app_config.h"
#include "deck_actions.h"

struct DeckTile {
  char           label[28];          // Beschriftung, '\n' fuer eine zweite Zeile
  uint32_t       color;              // 0xRRGGBB
  DeckActionType type;
  char           spec[DECK_SPEC_MAX];
  char           icon[24];           // Icon-ID (Datei /icons/<id>.bin), leer = kein Icon
  uint8_t        slot;               // Platz im Raster (0 = links oben, zeilenweise); Luecken zwischen den Kacheln sind erlaubt
};

struct DeckProfile {
  char     id[24];                   // Dateiname ohne ".json" (a-z, 0-9, '-', '_')
  char     name[32];                 // Anzeigename
  char     exe[48];                  // Programmname fuer den spaeteren Auto-Wechsel (z. B. "bambu-studio.exe")
  uint8_t  cols, rows;
  uint8_t  theme;                    // veraltet (bis v0.4.0 pro Profil); wird nur noch gelesen, das Design gilt jetzt geraeteweit
  uint8_t  brightness;               // veraltet (bis v0.4.0 pro Profil); wird nur noch gelesen, die Helligkeit gilt jetzt geraeteweit
  uint8_t  tileCount;                // Anzahl der Kacheln (tiles[0..tileCount-1]); ihr Platz steht in tile.slot, freie Plaetze bleiben leer
  DeckTile tiles[DECK_MAX_TILES];
};

struct ProfileInfo { char id[24]; char name[32]; char exe[48]; };

bool  profilesInit();                              // Dateisystem mounten, Beispielprofile anlegen, aktives laden
bool  profilesStorageOk();                         // false: Flash-Speicher nicht nutzbar (RAM-Notprofil aktiv)
int   profileCount();
const ProfileInfo& profileInfo(int index);
int   profileActiveIndex();
const DeckProfile& profileActive();
bool  profileActivate(int index);                  // laden; die Auswahl wird verzoegert gespeichert (siehe profilesTick)
bool  profilesHostActions();                       // Tasten vom PC-Helfer statt ueber die USB-Tastatur ausfuehren (Standard: aus)
void  profilesSetHostActions(bool on);
bool  profilesAutoSwitch();                        // automatischer Profilwechsel nach dem Vordergrund-Programm (Standard: aus)
void  profilesSetAutoSwitch(bool on);              // wird verzoegert gespeichert
int   profileIndexByExe(const char* exe);          // Profil zu einem Programmnamen (ohne Beachtung von Gross-/Kleinschreibung und ".exe"), -1 = keins
void  profilesTick();                              // regelmaessig aufrufen (1 s): schreibt die Auswahl, wenn 8 s nichts gewechselt wurde
void  profileNamesJoined(String& out);             // "A\nB\nC" fuer das Roller-Widget

// Helligkeit fuer das ganze Geraet (3..100 %); wird verzoegert gespeichert
uint8_t profilesBrightness();
void  profilesSetBrightness(uint8_t percent);
// Design fuer das ganze Geraet: 0 = hell, 1 = dunkel (ThemesEnum aus PicoPixel); wird verzoegert gespeichert
uint8_t profilesTheme();
void  profilesSetTheme(uint8_t theme);

// Schnellwahl P1..P5 auf dem Deck-Bildschirm: je Taste eine Profil-ID (leer = nicht belegt), geraeteweit, vom Editor gesetzt.
// Die Taste "Standard" springt immer auf das Profil "standard".
#define DECK_QUICK_COUNT 5
const char* profilesQuickId(int n);                // n = 0..4; "" wenn nicht belegt oder das Profil nicht (mehr) existiert
void  profilesSetQuick(int n, const char* id);     // "" = frei; wird verzoegert gespeichert

// Fuer den spaeteren Import ueber CDC/PC-Programm
bool  profileLoad(const char* id, DeckProfile& out);
bool  profileSave(const DeckProfile& p);
bool  profileDelete(const char* id);

// Fuer die Befehle vom PC (CDC). Nur Datei-Zugriffe - das aktive Profil im RAM aendert ausschliesslich der LVGL-Task.
void  profilesRescan();                                   // Dateiliste neu lesen; aktives Profil bleibt (per id) erhalten
int   profileIndexOf(const char* id);                     // -1 = unbekannt
void  profileListJson(String& out);                       // {"active":"id","profiles":[{"id","name","exe"},...]}
bool  profileReadRaw(const char* id, String& out);        // Dateiinhalt (eine Zeile JSON)
bool  profileImportJson(const char* id, const char* json, const char*& err);   // pruefen und speichern
