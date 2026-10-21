#pragma once
// Hilfsprogramme, die vom Einstellungs-Screen aus geöffnet werden. Ihre Screens entstehen erst beim
// Öffnen im Code (nicht in PicoPixel) und werden beim Schließen wieder freigegeben.
void uiRechnerInit();   // Taschenrechner  (Button "buttoncalc")
void uiTimerInit();     // Küchentimer + Stoppuhr (Button "buttonclock")
