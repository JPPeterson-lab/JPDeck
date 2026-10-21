#pragma once
// Kommandokanal ueber die virtuelle COM-Schnittstelle. Zeilenbasiert (Abschluss \n oder \r\n).
// Diese Schicht kennt nur den Kanal selbst (und PING). Alle weiteren Befehle meldet die App-Schicht
// ueber usbCdcRegister() an (siehe app/host_commands.cpp).
typedef void (*UsbCdcHandler)(const char* args);      // args = Rest der Zeile nach dem Befehlswort

void usbCdcInit();                                    // Lese-Task starten (nach usbDeviceInit)
bool usbCdcRegister(const char* name, UsbCdcHandler fn);   // Name in Grossbuchstaben, max. 16 Befehle
void usbCdcSendLine(const char* line);                // eine Zeile an den PC schicken
