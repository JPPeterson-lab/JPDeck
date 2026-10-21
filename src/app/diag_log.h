#pragma once
// Diagnose-Log: Ringpuffer mit den letzten Ereignissen (Start, USB, Profile, Flash-Schreibzugriffe, Aktionen).
// Jede Zeile geht zusätzlich auf den UART-Port. Abrufbar ueber den COM-Port mit "LOG" und "STATUS".
#include <Arduino.h>

void logDiag(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void diagFlashWritten();                                 // nach jedem Flash-Schreibzugriff aufrufen (zaehlt mit)
uint32_t diagFlashCounter();
void logDump(void (*emit)(const char* line));            // alle gespeicherten Zeilen, aelteste zuerst
void logStatus(char* out, size_t n);                     // eine Zeile mit Speicher, Anzeige und USB-Zustand
