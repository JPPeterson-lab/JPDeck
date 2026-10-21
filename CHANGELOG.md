# Changelog

*[English version](CHANGELOG.en.md)*

## v0.6.1 (2026-10-16)

### Neu
- **Fünf Schnelltasten statt drei:** **P1–P5** lassen sich im Editor frei auf Profile legen (Bereich „Schnellwahl am Gerät“ mit fünf Auswahlfeldern). Der CDC-Befehl `QUICK` nimmt jetzt fünf Profil-IDs (`QUICK <id1> … <id5>`, `-` = frei). Eine bestehende Belegung von P1–P3 bleibt erhalten, P4 und P5 sind zunächst frei
- Neue UI aus PicoPixel

### Geändert
- **Symbole der Oberfläche jetzt aus den [Bootstrap Icons](https://icons.getbootstrap.com/)** (MIT) statt aus den Flat Color Icons von Icons8; Credits, README und Installer-Seite angepasst

## v0.6.0 (2026-10-16) – Beta

Das Projekt wechselt von **Alpha** zu **Beta**: der Funktionsumfang steht, Kernfunktionen sind am Gerät und am Windows-PC geprüft.

### Neu
- **Schnelltasten auf dem Deck-Bildschirm** (neue UI aus PicoPixel): **Standard** springt von jedem Profil zurück auf das Profil `standard`. **P1–P3** springen auf ein frei wählbares Profil. Sie liegen neben dem Kachelraster und sind damit auf jeder Profilseite sichtbar und bedienbar. Nicht belegte Tasten sind blass und ohne Funktion, das aktive Profil ist blau umrandet
- **Belegung im Editor:** Bereich „Schnellwahl am Gerät“ mit je einer Auswahl für P1–P3 (wirkt sofort). Gespeichert wird geräteweit in `config.json` (`quick`), `LIST` liefert sie mit; neuer CDC-Befehl `QUICK <id1> <id2> <id3>` (`-` = frei). Wird ein Profil gelöscht, wird die Taste frei
- **Windows-Helfer als richtiges Programm:** `DeckPad-Helper.exe` (C#, WinForms, .NET Framework 4.8, ca. 80 KB) ersetzt das PowerShell-Skript. Statusfenster, Symbol im Infobereich, nur eine Instanz. Optionen (beide standardmäßig **aus**): *Mit Windows starten (im Hintergrund)* (Autostart pro Benutzer, kein Dauerbetrieb ohne Zustimmung) und *Beenden, wenn das DeckPad getrennt wird*. Quellcode in `tools/helper-app/`, Bauen mit `tools/build_helper.sh`. Die Datei ist nicht signiert (SmartScreen-Hinweis)

### Geändert
- Neue UI-Exports (Schriftgröße 62 entfällt, 92 neu, Icons für die Schnelltasten)

## v0.5.0 (2026-10-15)

### Neu
- **Kacheln frei im Raster anordnen:** jede Kachel hat einen festen Platz (`slot`), Lücken sind erlaubt. Im Editor Kacheln **ziehen**: auf einen leeren Platz verschiebt, auf eine belegte Kachel tauscht beide. Beim Ändern der Rastergröße behalten die Kacheln ihre Zeile und Spalte, was nicht mehr hineinpasst, fällt (nach Rückfrage) weg. Ältere Profile ohne `slot` laden unverändert (Reihenfolge)
- **Neue UI aus PicoPixel** (andere Farben, Container verschoben, größerer Profil-Roller); das Zusatzfeld im Einstellungs-Screen fluchtet jetzt mit den Containern

### Geändert
- **Design (hell/dunkel) und Helligkeit gelten für das ganze Gerät**, nicht mehr pro Profil. Vorher änderte der Schalter nur das Design des gerade aktiven Profils, und neue Profile starteten immer hell. Beide Werte stehen jetzt in `config.json` und werden verzögert gespeichert (kein Flash-Schreibzugriff mehr bei jedem Verstellen). Beim Update übernimmt das Gerät Design und Helligkeit des aktiven Profils. Aus dem Editor sind die beiden Regler entfallen, aus den Profildateien die Einträge `settings`
- **Komplett auf ESP-IDF umgestellt, Arduino IDE entfällt:** das IDF-Projekt liegt jetzt im Wurzelordner (`bash build.sh`, `main/main.cpp`), `platforms/`, `Deckpad.ino`, `build_opt.h` und die Arduino-USB-Schicht sind entfernt. Arduino-ESP32 bleibt als Komponente für `Serial`, LittleFS und ArduinoJson
- Das P4-Board wird nicht für das DeckPad eingesetzt (kommt in ein Wetterstation-Projekt), das DeckPad bleibt beim ESP32-S3

## v0.4.0 (2026-10-13)

### Neu
- **ESP-IDF-Build (`platforms/idf/`) als Hauptweg:** gleicher Quellcode, aber mit eigener Konfiguration statt des vorkompilierten Arduino-Cores. Display-Ansteuerung flash-sicher (`LCD_RGB_ISR_IRAM_SAFE`, `GDMA_ISR_IRAM_SAFE`), PSRAM 120 MHz, Code im PSRAM, 64-KB-Daten-Cache. **Die Bildstreifen beim Speichern treten nicht mehr auf**, die Schreibzeiten sinken (29–93 ms statt 68–199 ms). Bauen und Flashen: `platforms/idf/build.sh`
- **Web-Installer** (`docs/index.html`, GitHub Pages): ein Klick statt Entwicklungsumgebung, mit [esp-web-tools](https://esp.huhn.me/). Das Firmware-Image (Bootloader, Partitionstabelle und Programm in einer Datei) liegt unter `docs/firmware/`; `tools/prepare_release.sh` baut es und schreibt `manifest.json`. Der Editor liegt jetzt unter `…/editor.html`
- **Pixeltakt 30 MHz** (vorher 15 MHz): ca. 33 statt 16 Bilder/s, das Flimmern ist weg. Einstellbar (`PCLK=40 bash build.sh`, `DECK_PIXEL_CLOCK_HZ`)
- **Eigene USB-Schicht für IDF** (`src/usb/usb_device_idf.cpp`, `esp_tinyusb`): HID-Tastatur + Medientasten + COM-Port, gleiche Schnittstelle wie im Arduino-Build
- **Messwerte im Log:** pro Aktion Wartezeit und Sendedauer (`[Aktion] …: Warten x ms, Senden y ms`), längste Runde der Oberfläche in der `[Diag]`-Zeile

### Geändert
- **Eigene USB-Produkt-ID `303A:4DC0`** (beide Builds). 303A:1001 gehört der USB-JTAG-Schnittstelle der Chips; deren Windows-Treiber verdrängte unter IDF die Tastatur (Schnittstelle MI_02 wurde als WinUSB statt HID eingerichtet). Windows legt das Gerät beim ersten Anschließen neu an (neue COM-Nummer). Der Helfer sucht zuerst die neue ID
- **Programmstart (`run`) und Text tippen deutlich schneller:** HID-Abfrageintervall 1 ms (vorher 10 ms), keine zusätzlichen Pausen zwischen den Zeichen: ca. 3 ms statt 13–15 ms pro Zeichen. Ein langer Chrome-Befehl dauert 1,2 s statt 2,2 s (davon 0,7 s Wartezeit für den Run-Dialog)

### Hinweis
- **Flashen:** Beim Flashen über den UART-Port darf das Kabel des nativen USB-Ports nicht gleichzeitig an einem anderen Rechner hängen (sonst bricht der Schreibvorgang ab)

## v0.3.3 (2026-10-13)

### Neu
- **Panel-Neustart:** Nach jedem Flash-Schreibzugriff startet das Gerät zusätzlich die Display-Ansteuerung neu (`esp_lcd_rgb_panel_restart`, laut Espressif-Doku gegen eine dauerhafte Verschiebung, wenn LCD-Controller und DMA aus dem Takt geraten sind) und zeichnet den Bildschirm neu. Dazu COM-Befehl `PANEL` und Editor-Knopf **„Panel neu starten“** zum Testen. Ob das die Streifen beseitigt, ist noch nicht bestätigt

## v0.3.2 (2026-10-13)

### Neu
- **Display neu zeichnen:** Knopf im Editor und COM-Befehl `REDRAW`. Zeichnet das ganze Display neu. Ist ein Bildstreifen danach weg, war nur der Bildinhalt gestört; bleibt er, liegt es an der Display-Ansteuerung selbst. Dient der Eingrenzung der Streifen am linken Rand
- **Automatisches Neuzeichnen nach Flash-Schreibzugriffen** (Profil speichern, Icon hochladen, Auswahl sichern): Der RGB-Treiber ist im vorkompilierten Core nicht IRAM-sicher (`CONFIG_LCD_RGB_ISR_IRAM_SAFE` nicht gesetzt), die Ansteuerung setzt beim Schreiben kurz aus. Danach wird einmal der ganze Bildschirm neu gezeichnet; im Log als `[UI] Vollbild neu gezeichnet`

## v0.3.1 (2026-10-13)

### Neu
- **„Tasten am PC ausführen“:** zweiter Schalter im Einstellungs-Screen (Standard: aus). Dann tritt das Board nicht als USB-Tastatur auf, sondern schickt jede Aktion als `DO <typ> <angabe>` über den COM-Port, und der **Windows-Helfer** führt Tasten, Medientasten, Text und Programmstarts aus (`SendInput`, `Start-Process`). Gedacht für Boards ohne USB-Tastatur (z. B. das geplante ESP32-P4-Board) und als Ausweichweg. Ohne Helfer passiert nichts (Hinweis im Log und am Display)
- Der Helfer hört jetzt alle 40 ms auf das Gerät (vorher nur 0,6 s), das Vordergrundfenster wird weiter alle 0,6 s geprüft

### Behoben
- **Tastenkürzel mit Z oder Y waren auf deutschen Tastaturen vertauscht:** Buchstaben und Ziffern wurden als *physische* Taste der US-Belegung gesendet, auf deutscher Belegung ist das Z dort das Y (`ctrl+z` kam als Strg+Y an). Jetzt folgen Buchstaben und Ziffern dem Layout des PCs (`DECK_KEY_LAYOUT`, Standard Deutsch). Betrifft z. B. das Beispielprofil „Bambu Studio“ (Rückgängig). Strg+Shift+M, Strg+C/V, Strg+S waren nicht betroffen. Wer einen PC mit US-Belegung nutzt, stellt `DECK_KEY_LAYOUT` auf `KeyboardLayout_en_US`

## v0.3.0 (2026-10-13)

### Neu
- **Icons auf den Kacheln:** Symbole (Emoji-Auswahl) oder eigene Bilder (PNG, JPG, SVG …), im Editor in 32–80 px umgewandelt und per COM-Port ins Gerät geladen (RGB565 + Alpha, `/icons/<id>.bin`, bis 96 × 96 px). Mehrere Profile können dasselbe Icon nutzen. Die Schnellauswahl im Editor bringt passende Symbole mit. COM-Befehle `ICONBEGIN`, `ICONDATA`, `ICONEND`, `ICONLIST`, `ICONGET`, `ICONDEL`
- **Automatischer Profilwechsel (einstellbar):** Schalter „Automatischer Profilwechsel“ im Einstellungs-Screen (Standard: aus). Ein kleiner **Windows-Helfer** (`tools/helper/deckpad-helper.ps1`, ohne Installation) meldet das Programm im Vordergrund; das DeckPad wählt das Profil mit passendem Feld „Programm“ (z. B. `bambu-studio.exe`), sonst „Standard“. Der Einstellungs-Screen zeigt, ob der Helfer erkannt wird. COM-Befehl `ACTIVE <programm.exe>`
- **Editor:** Knopf **„Trennen“** gibt den COM-Port frei (der Helfer gibt ihn außerdem von selbst frei, solange der Editor im Vordergrund ist)
- **Statusanzeige:** „PC verbunden“ ist grün hinterlegt, „Kein PC“ rot

## v0.2.1 (2026-10-13)

### Geändert
- **Profilauswahl wird verzögert gespeichert:** bisher schrieb jeder Profilwechsel sofort in den Flash. Flash-Schreibzugriffe stoppen kurz den Cache (und damit den PSRAM-Zugriff des Displays) und verschleißen den Flash. Jetzt wird die Auswahl erst geschrieben, wenn 8 s lang nicht gewechselt wurde (`[Flash]`-Zeile im Log). Mögliche Ursache der Streifen nach mehreren Profilwechseln, noch nicht bestätigt

### Neu
- **Mehr Messwerte im Log:** Dauer des Kachel-Aufbaus bei jedem Profilwechsel, und einzelne langsame Flushes (länger als 20 ms) mit Größe und Position des Bereichs

## v0.2.0 (2026-10-13)

### Neu
- **DeckPad Editor** (`tools/editor/deckpad-editor.html`): Profile am **Windows-PC** im Browser (Chrome/Edge, Web Serial) anlegen, bearbeiten, kopieren, löschen und aktivieren. Kacheln anklicken, Beschriftung, **Farbe** (Farbwähler und Palette), **Aktion** (Medien, Tastenkombination mit Modifier-Kästchen und Tastenaufnahme, Programm starten, Text, Host-Nachricht) und Schnellauswahl-Vorlagen. Profil-Sicherung als Datei, Demo-Modus ohne Gerät. Setzt beim Verbinden die **Uhrzeit** des PCs. Zeigt Status und Geräte-Log
- **Editor online:** über GitHub Pages erreichbar (<https://jppeterson-lab.github.io/DeckPad/editor.html>), nichts zu installieren. `tools/deploy_editor.sh` kopiert die Datei nach `docs/index.html`
- **Editor erkennt den falschen Port:** der Auswahldialog zeigt nur Espressif-Geräte (VID 0x303A), also den nativen USB-Port; der UART-Port (CH343) wird erkannt und erklärt. „Alle Ports anzeigen“ hebt den Filter auf
- **Aktion `run`:** startet ein Programm, eine Datei oder einen Link über Win+R (Programmname, Pfad, Internetadresse). Das Gerät drückt Win+R, tippt den Text und bestätigt mit Enter
- **COM-Befehle:** `VER`, `LIST`, `GET <id>`, `PUT <id> <json>`, `DEL <id>`, `USE <id>`, `TIME <Unix-Sekunden>` (zusätzlich zu `PING`, `STATUS`, `LOG`). Das Gerät übernimmt Änderungen sofort (Roller, Raster, Farben)
- **Schichten:** die USB-Schicht kennt nur den Kanal; Befehle werden von `app/host_commands.cpp` angemeldet

### Geändert
- Zeilenlänge der COM-Befehle von 256 auf 6144 Byte, Stack des CDC-Tasks auf 8 KB (ein Profil als JSON braucht bis ca. 4 KB)

## v0.1.1 (2026-10-13)

### Neu
- **Diagnose-Log:** Ringpuffer mit den letzten 64 Ereignissen (Start inkl. Reset-Grund und Display-Einstellungen, USB-Ereignisse, Tastendrücke, Profilwechsel, **Flash-Schreibzugriffe mit Dauer**). Jede Zeile geht zusätzlich auf den UART-Port
- **COM-Befehle `STATUS` und `LOG`:** `STATUS` liefert eine Zeile mit Version, Laufzeit, freiem und minimalem Speicher (SRAM, PSRAM) und den Flush-Zeiten; `LOG` gibt den Ringpuffer aus
- **Flush-Messung:** Anzahl, mittlere und maximale Dauer des Kopierens in den PSRAM-Framebuffer; alle 10 s eine `[Diag]`-Zeile auf dem UART-Port. Zweck: Streifen und zerrissene Zeilen (PSRAM-Bandbreite) mit Messwerten statt Vermutungen eingrenzen

## v0.1.0 (2026-10-13)

Erstes Grundgerüst. Der Code kompiliert, ein Test am Gerät steht noch aus.

### Neu
- **Projekt:** eigenständiges Projekt (Treiber für RGB-Panel, GT911, I²C, IO-Expander, LVGL-Teilrendering, `build_opt.h`)
- **Kachelraster:** die Kacheln entstehen im Code im Platzhalter `ContainerGrid` aus dem PicoPixel-Entwurf. Standard 5 × 3, pro Profil bis 6 × 4. Beim Antippen wechselt die Kachel sofort die Farbe und blendet in 350 ms zurück
- **USB zum PC:** nativer USB-Port als Composite-Gerät mit HID-Tastatur, Consumer/Media-Tasten und virtueller COM-Schnittstelle. `Serial` bleibt auf dem USB-C-UART-Port (Flashen und Debug)
- **USB/CAN-Umschaltung:** IO5 des IO-Expanders wird nach der Display-Init explizit auf USB gesetzt (der Treiber hätte ihn auf CAN stehen lassen)
- **CDC:** zeilenbasierter Befehlsleser mit Befehlstabelle; derzeit antwortet nur `PING` mit `PONG`
- **Aktionen:** `media`, `keys` (z. B. `ctrl+shift+m`), `text`, `host`; die Ausführung läuft in einem eigenen Task, damit LVGL nie auf USB wartet
- **Profile:** je Programm eine JSON-Datei auf der LittleFS-Partition mit eigenen Kacheln und eigenen Einstellungen (Theme, Helligkeit, Raster). Beispielprofile „Standard“ und „Bambu Studio“ beim ersten Start. Auswahl über den Roller in den Einstellungen und die Pfeil-Tasten auf dem Deck
- **Einstellungen pro Profil:** Theme und Helligkeit werden beim Verstellen im aktiven Profil gespeichert (Helligkeit beim Loslassen, um den Flash zu schonen)
- **Taschenrechner**, geöffnet über das Rechner-Symbol in den Einstellungen
- **Küchentimer und Stoppuhr:** Timer mit Zeittasten läuft im Hintergrund weiter; bei Ablauf blinkt das Display (Bildschirm und Hintergrundbeleuchtung) auf jedem Screen, bis man es antippt (spätestens nach einer Minute). Stoppuhr mit Rundenzeiten
- **Partitionstabelle ohne OTA:** 4 MB Factory-App, 24 KB NVS, 64 KB Coredump, 11,9 MB LittleFS
- **Werkzeuge:** `tools/deploy_ui.sh` (PicoPixel-Export aufbereiten und prüfen), `tools/bump_version.sh`
- **Doku:** README, Changelog und DEVLOG zweisprachig, Wiki

### Geändert gegenüber dem PicoPixel-Entwurf
- Die 15 statischen Kachel-Container auf dem Deck-Screen wurden durch **einen** Platzhalter-Container `ContainerGrid` (27, 87, 950 × 384) ersetzt. In PicoPixel müssen die alten Container 1–6 und 9–17 noch gelöscht werden; `deploy_ui.sh` meldet sie, solange sie existieren

### Bekannt
- Das Verbindungs-Ereignis am Gerät ist noch nicht bestätigt (die Polarität von IO5, `0` = USB, bestätigt die Waveshare-Dokumentation)
- Uhrzeit zeigt `--:--`, bis ein späterer Schritt sie vom PC setzt
