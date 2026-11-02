#pragma once
// Streifen-Seiten: eine Touch-Flaeche, auf der nur EINE Richtung zaehlt, wie ein grosses Scrollrad oder ein Jog-Rad.
//   scroll    senkrecht ziehen  -> Mausrad (Dokumente, Webseiten, CAD-Zoom)
//   hscroll   waagerecht ziehen -> waagerechtes Mausrad
//   timeline  waagerecht ziehen -> Pfeiltasten links/rechts (Einzelbilder in Videoschnitt und Playern); 2 Finger = Shift (grosse Schritte)
//   zoom      senkrecht ziehen  -> Strg + Mausrad (Zeitleisten und Bilder zoomen)
// Der erste Kontakt ist der Nullpunkt, die Seite ist also blind bedienbar. Aufbau wie nav_pad.cpp.
void stripPadOpen(const char* mode);        // leer/unbekannt = scroll
bool stripPadModeValid(const char* mode);   // leer oder eine der Kennungen oben
