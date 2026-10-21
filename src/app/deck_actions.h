#pragma once
// Aktionen, die eine Kachel am PC ausloest. Die UI ruft nur deckActionRun() auf - sie weiss nicht,
// wie USB funktioniert. Die Ausfuehrung laeuft in einem eigenen Task, damit LVGL nie auf USB wartet.
#include <stdint.h>
#include <stddef.h>

enum DeckActionType : uint8_t {
  ACT_NONE = 0,   // Kachel ohne Funktion (nur Farbwechsel)
  ACT_MEDIA,      // Media-/Lautstaerketaste, spec z. B. "vol_up", "mute", "play_pause"
  ACT_KEYS,       // Tastenkombination, spec z. B. "ctrl+shift+m"
  ACT_TEXT,       // Text tippen, spec = der Text
  ACT_HOST,       // Nachricht an ein PC-Programm ueber CDC ("ACTION <spec>"), fuer spaeter
  ACT_RUN,        // Programm/Datei/Link starten: Win+R, Text tippen, Enter. spec z. B. "notepad" oder "C:\\Programme\\x.exe"
  ACT_LAST = ACT_RUN
};

#define DECK_SPEC_MAX 96

void deckActionsInit();                                           // Task + Warteschlange anlegen
bool deckActionRun(DeckActionType type, const char* spec);        // nicht blockierend; false = Queue voll
bool deckActionValid(DeckActionType type, const char* spec);      // Syntaxpruefung (z. B. fuer Profil-Import)

const char*    deckActionTypeName(DeckActionType t);              // "none" | "media" | "keys" | "text" | "host" | "run"
DeckActionType deckActionTypeFromName(const char* name);          // unbekannt -> ACT_NONE
