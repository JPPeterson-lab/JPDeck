#pragma once
// COM-Befehle fuer den PC (Editor im Browser, Terminal): Profile lesen/schreiben/waehlen, Uhrzeit, Diagnose.
// Verbindet die USB-Schicht (usb_cdc) mit Profilen und UI. Einmal nach usbCdcInit() und profilesInit() aufrufen.
void hostCommandsInit();
