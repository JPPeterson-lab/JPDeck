# 🎛️ JPDeck

![Status](https://img.shields.io/badge/Status-Beta-yellow?style=flat-square)
![Version](https://img.shields.io/badge/Version-v0.7.0-blue?style=flat-square&cacheSeconds=1)
![Hardware](https://img.shields.io/badge/Hardware-Waveshare%20ESP32--S3--Touch--LCD--7B-red?style=flat-square&logo=espressif&logoColor=white)
![Display](https://img.shields.io/badge/Display-7%22%201024×600%20RGB-informational?style=flat-square)
![Lizenz](https://img.shields.io/badge/Lizenz-MIT-blue?style=flat-square)
![Plattform](https://img.shields.io/badge/Platform-ESP--IDF-teal?style=flat-square&logo=espressif&logoColor=white)

**JPDeck** (bis v0.6.1 „DeckPad“, umbenannt, weil der Name schon vergeben war) ist ein **Stream-Deck-artiges Touch-Pad** auf dem Waveshare ESP32-S3-Touch-LCD-7B (7", 1024×600, kapazitiver Touch). Die Kacheln auf dem Display lösen am Windows-PC Aktionen aus: Lautstärke, Medientasten, beliebige Tastenkombinationen, Programmstarts. Der PC erkennt das Board über den nativen USB-Port als **Tastatur (inkl. Media-Tasten) plus COM-Port**. Alle Kacheln entstehen im Code aus **Profilen**, jedes Programm bekommt sein eigenes Profil mit eigenen Tasten, die sich frei im Raster anordnen lassen.

Das Projekt ist ein **ESP-IDF-Projekt** (Arduino-ESP32 läuft als Komponente für `Serial`, LittleFS und ArduinoJson), ohne WLAN und ohne OTA.

> **🟡 Beta (v0.7.0):** Der Kern ist fertig und läuft am Gerät und am Windows-PC (Tastatur, Medientasten, COM-Port, Editor, Icons, freie Kachelplätze, Schnellwahl). Keine Bildstreifen beim Speichern, kein Flimmern. Noch nicht ausführlich getestet sind Rechner, Timer, der Windows-Helfer und der Helfer-Modus; Dateiformate und Befehle können sich bis zur 1.0 noch ändern. Fehler bitte als [Issue](https://github.com/JPPeterson-lab/JPDeck/issues) melden.

---

## 🚀 Installation

**Empfohlen: Web-Installer** – <https://jppeterson-lab.github.io/JPDeck/>. In Chrome oder Edge öffnen, das Board über die Buchse **„UART“** anschließen (die Buchse „USB“ muss dabei frei sein), „Installieren“ klicken und den Port „USB-Enhanced-SERIAL CH343“ wählen. Danach das Kabel der Buchse **„USB“** mit dem PC verbinden. „Gerät löschen“ löscht auch gespeicherte Profile und Icons; ohne diese Option bleiben sie erhalten (Update).

> **Selbst bauen** ist möglich, setzt aber **gute Kenntnisse im ESP32-Umfeld** voraus (ESP-IDF, Partitionen, Flash-Optionen, Treiber): Das ESP-Ökosystem ist komplex. Wer das nicht braucht, nutzt den Web-Installer. Die frühere Arduino-IDE-Variante gibt es nicht mehr, das Projekt ist komplett auf ESP-IDF umgestellt.

### Selbst bauen (für Erfahrene)
1. **ESP-IDF 5.5.x** installieren: `git clone --branch v5.5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf-v5.5.5 && cd ~/esp/esp-idf-v5.5.5 && ./install.sh esp32s3`
2. Board über den **UART-Port** anschließen, im Projektordner:
```bash
bash build.sh flash /dev/cu.usbmodemXXXX     # Port des UART-Anschlusses (CH343)
```
3. Danach den **nativen USB-Port** mit dem PC verbinden. **Beim Flashen darf dieser nicht gleichzeitig an einem anderen Rechner hängen** (sonst bricht das Schreiben ab). Windows richtet das Gerät beim ersten Anschließen neu ein (eigene USB-ID `303A:4DC0`).

Weitere Befehle: `bash build.sh` (nur bauen), `bash build.sh clean`, Pixeltakt `PCLK=40 bash build.sh flash …` (Standard 30 MHz, ca. 33 Bilder/s). Release-Image für den Web-Installer: `bash tools/prepare_release.sh`.

---

## 📊 Status

| Baustein | Status |
|---|---|
| RGB-Display 1024×600, GT911-Touch, LVGL-Teilrendering, Pixeltakt 30 MHz | ✅ am Gerät geprüft |
| Display-Ansteuerung flash-sicher (keine Streifen beim Speichern) | ✅ am Gerät geprüft |
| USB-Verbund: HID-Tastatur + Medientasten + COM-Port (eigene USB-ID) | ✅ am PC geprüft |
| Kachelraster (5×3, bis 6×4), Farbwechsel beim Antippen | ✅ geprüft |
| **Kacheln frei im Raster anordnen** (Lücken erlaubt, Ziehen im Editor) | ✅ geprüft |
| **Schnellwahl: Taste „Standard“ und P1–P5** (im Editor belegbar, auf jeder Profilseite) | 🧪 neu in v0.6.0 (P1–P5 seit v0.6.1), ungetestet |
| Profile im Flash (JSON pro Profil), Auswahl per Roller und Pfeil-Tasten | ✅ geprüft |
| **Design und Helligkeit gelten für das ganze Gerät** | ✅ geprüft |
| JPDeck Editor (Web Serial, Farben, Aktionen, Icons) | ✅ am Gerät geprüft |
| Icons auf Kacheln (Emoji oder eigene Bilder) | ✅ am Gerät geprüft |
| Programmstart (`run`) | ✅ geprüft |
| Uhrzeit vom PC (setzt der Editor) | ✅ geprüft |
| Taschenrechner, Küchentimer/Stoppuhr | 🧪 ungetestet |
| Automatischer Profilwechsel (Schalter + Windows-Helfer) | 🧪 ungetestet |
| Aktionen über den PC-Helfer statt USB-Tastatur | 🧪 ungetestet |
| Web-Installer (GitHub Pages) | ✅ Installation am Gerät geprüft (Update ohne „Gerät löschen“) |

---

## 🎬 Screens

| Screen | Inhalt |
|---|---|
| **Deck** | Uhrzeit, PC-Status (grün/rot), Profilname, Kachelraster, Pfeile zum Profilwechsel, Schnelltasten Standard/P1–P5, Zahnrad |
| **Einstellungen** | Helligkeit, Design (hell/dunkel), Profil-Roller, Version, Schalter für Auto-Profilwechsel und „Tasten am PC ausführen“, Rechner und Timer |
| **Rechner** | Vier Grundrechenarten, Prozent, Vorzeichen |
| **Timer / Stoppuhr** | Küchentimer mit Zeittasten (läuft im Hintergrund, bei Ablauf blinkt das Display), Stoppuhr mit Rundenzeiten |
| **Boot** | Statuszeile und Balken beim Start |

---

## 🎹 Profile und Kacheln

Jedes Profil ist eine JSON-Datei `/profiles/<id>.json` auf der Datenpartition: Name, Programm, Raster und die **Kacheln**. Jede Kachel hat einen festen **Platz** (`slot`, 0 = links oben, zeilenweise). Freie Plätze bleiben leer, die Kacheln können also beliebig im Raster liegen. **Design (hell/dunkel) und Helligkeit** stehen nicht im Profil, sie gelten für das ganze Gerät.

```json
{
  "v": 2,
  "name": "Bambu Studio",
  "exe": "bambu-studio.exe",
  "cols": 5, "rows": 3,
  "tiles": [
    { "slot": 0, "label": "Speichern", "color": "#27ae60", "type": "keys",  "spec": "ctrl+s" },
    { "slot": 7, "label": "Lauter",    "color": "#2980b9", "type": "media", "spec": "vol_up", "icon": "e1f50a-48" }
  ]
}
```

| `type` | `spec` | Wirkung |
|---|---|---|
| `none` | – | nur Farbwechsel |
| `media` | `vol_up`, `vol_down`, `mute`, `play_pause`, `next`, `prev`, `stop`, `calculator`, `browser`, `mail` | Medien-/Consumer-Taste |
| `keys` | z. B. `ctrl+shift+m` (Teile mit `+`) | Tastenkombination; Modifier `ctrl shift alt win altgr`, Tasten `a-z 0-9 f1-f24 enter esc tab space backspace delete insert home end pageup pagedown up down left right printscreen pause capslock` |
| `text` | beliebiger Text | wird getippt (ASCII, Layout des PCs) |
| `run` | `notepad`, `C:\Pfad\x.exe`, `https://…` | startet Programm/Datei/Link über Win+R |
| `host` | frei | geht als `ACTION <spec>` über die COM-Schnittstelle an ein PC-Programm |

Beim ersten Start legt das Gerät zwei **Beispielprofile** an: **Standard** und **Bambu Studio**. Gewechselt wird über den **Roller** in den Einstellungen oder die **Pfeil-Tasten** auf dem Deck. Unter dem Kachelraster liegen die **Schnelltasten**: **Standard** springt von jedem Profil zurück auf das Profil „standard“, **P1–P5** springen auf ein Profil, das Sie im Editor (Bereich „Schnellwahl am Gerät“) zuweisen. Sie sind auf jeder Profilseite sichtbar; nicht belegte Tasten sind blass, das aktive Profil ist blau umrandet.

### Profile am PC erstellen (JPDeck Editor)
Auf dem Windows-PC in **Chrome oder Edge** <https://jppeterson-lab.github.io/JPDeck/editor.html> öffnen (auch als [Einzeldatei](https://raw.githubusercontent.com/JPPeterson-lab/JPDeck/main/tools/editor/jpdeck-editor.html) nutzbar), **„Mit JPDeck verbinden“** wählen (Board über „USB“ angeschlossen, der COM-Port darf nirgends sonst offen sein). Dort:
- Profil wählen oder **Neu** / **Kopie** anlegen, Name, Programm und Raster einstellen,
- Kacheln **anklicken** (Beschriftung, **Farbe**, **Aktion**, **Icon**) und **ziehen**: auf einen leeren Platz ziehen verschiebt, auf eine belegte Kachel ziehen tauscht beide,
- **„Auf JPDeck speichern“**; das Gerät übernimmt das Profil sofort.

Der Editor setzt beim Verbinden die **Uhrzeit** des PCs und zeigt Status und Geräte-Log. Ohne Gerät lässt er sich mit **„Demo ohne Gerät“** ausprobieren. **Programm starten** (`run`): Das JPDeck drückt Win+R, tippt den Text und bestätigt mit Enter (Programmname, Pfad oder Internetadresse).

### Icons auf Kacheln
Im Editor **„Symbol wählen“** (Emoji) oder **„Bild laden …“** (PNG, JPG, SVG), Größe 32–80 px (mit Text darunter besser 48 px oder kleiner). Beim Speichern lädt der Editor neue Icons ins Gerät (bis 96 × 96 px, gemeinsam für alle Profile).

---

## 🔄 PC-Helfer (Windows)
Ein kleines Windows-Programm: [`JPDeck-Helper.exe`](https://jppeterson-lab.github.io/JPDeck/JPDeck-Helper.exe) herunterladen und doppelklicken, nichts zu installieren (.NET Framework 4.8 ist in Windows 10/11 enthalten). Es zeigt ein Fenster mit Status und läuft danach als Symbol im Infobereich der Taskleiste. Windows warnt bei der ungesignierten Datei (SmartScreen): **Weitere Informationen → Trotzdem ausführen**. Es gibt den COM-Port frei, solange der Editor im Vordergrund ist. Zwei Aufgaben, beide in den Einstellungen des Geräts schaltbar:
- **Automatischer Profilwechsel:** wählt das Profil nach dem Programm im Vordergrund (Feld „Programm“ im Profil, sonst „Standard“).
- **Tasten am PC ausführen:** führt Tasten, Medientasten, Text und Programmstarts am PC aus, statt dass das Board als USB-Tastatur auftritt (Ausweichweg).

Der Helfer läuft **nur, wenn Sie ihn starten**. Im Fenster gibt es zwei Optionen, beide standardmäßig aus: **Mit Windows starten (im Hintergrund)** und **Beenden, wenn das JPDeck getrennt wird**. Das Fenster zu schließen versteckt es nur, **Beenden** (Fenster oder Rechtsklick auf das Symbol) beendet den Helfer.

Der Helfer sendet nur den Namen des Vordergrundprogramms an das JPDeck und führt im zweiten Modus aus, was das angeschlossene JPDeck schickt: nur selbst eingesteckte Geräte verwenden.

---

## 🔌 Hardware

| Bauteil | Details |
|---|---|
| Board | [Waveshare ESP32-S3-Touch-LCD-7B](https://www.waveshare.com/esp32-s3-touch-lcd-7b.htm) – ESP32-S3, 16 MB Flash, 8 MB PSRAM (OPI) |
| Display | 7", 1024×600, 16-Bit-RGB-Interface |
| Touch | GT911 (I²C), kapazitiv |
| Backlight | PWM über den IO-Expander (I²C) |
| Zwei USB-C-Buchsen | **UART** (CH343): Flashen + Debug-Ausgabe · **USB** (nativ, GPIO19/20): Verbindung zum PC |

**GPIO19/20 teilen sich USB und CAN.** Über **IO5 des IO-Expanders** wird umgeschaltet (`0` = USB, von Waveshare bestätigt). Das JPDeck setzt IO5 nach der Display-Initialisierung auf USB (`DECK_USB_SELECT_LEVEL`), **CAN ist im JPDeck daher nicht nutzbar.**

**USB-Kennung:** `303A:4DC0` (eigene PID). `303A:1001` gehört der USB-JTAG-Schnittstelle der Chips; deren Windows-Treiber verdrängt dort die Tastatur. Für eine Veröffentlichung eine eigene PID beantragen (Espressif vergibt sie kostenlos).

### Speicher
| Bereich | Größe | Inhalt |
|---|---|---|
| `nvs` | 24 KB | Systemdaten |
| `factory` | 4 MB | Firmware (ca. 1 MB), **kein OTA** |
| `coredump` | 64 KB | Absturzprotokoll |
| `storage` (LittleFS) | 11,9 MB | Profile, Icons, Einstellungen |
| PSRAM (8 MB, 120 MHz) | – | Framebuffer, LVGL-Pool, Code und Konstanten |

### Projektstruktur
```
JPDeck/
├─ main/main.cpp            Einstieg (Reihenfolge des Starts)
├─ CMakeLists.txt, sdkconfig.defaults, partitions.csv, conf/lv_conf.h
├─ build.sh                 bauen / flashen
├─ tools/                   prepare_release.sh, deploy_ui.sh, deploy_editor.sh, bump_version.sh, editor/, helper-app/ (Windows-Helfer, C#), build_helper.sh
├─ docs/                    GitHub Pages: Web-Installer, Editor, Helfer, Firmware-Image
└─ src/
   ├─ hardware/   Waveshare-Treiber: RGB-Panel, GT911, I²C, IO-Expander, Backlight
   ├─ display/    LVGL-Anbindung (Teilrendering)
   ├─ usb/        USB-Gerät (esp_tinyusb: HID + CDC) und CDC-Befehle
   ├─ app/        Profile, Aktionen, UI-Logik (Kacheln), Navigation, Rechner, Timer, Icons
   └─ ui/         PicoPixel-Export
```
Die UI-Logik (`deck_ui.cpp`) kennt nur LVGL und die Profile. Eine Kachel ruft `deckActionRun()` auf; erst `deck_actions.cpp` spricht mit `usb/`. Nach jedem PicoPixel-Export `./tools/deploy_ui.sh` ausführen.

---

## 📚 Wiki
Ausführliche Seiten (Hardware, Inbetriebnahme, Profile, USB und CDC, Entwicklung): <https://github.com/JPPeterson-lab/JPDeck/wiki>

## 🙏 Credits
Siehe [CREDITS.md](CREDITS.md). GUI: [LVGL](https://lvgl.io). Oberfläche: [PicoPixel](https://picopixel.io), Icons: [Bootstrap Icons](https://icons.getbootstrap.com/). Treiber: Waveshare / Espressif.

## 📄 Lizenz
[MIT](LICENSE). Einzelne Dateien von Dritten behalten ihre Lizenz (Waveshare-/Espressif-Treiber: Apache-2.0, Tastaturlayouts: LGPL-2.1), siehe [NOTICE](NOTICE), [CREDITS.md](CREDITS.md) und `LICENSES/`.

---
---

# 🎛️ JPDeck (English)

**JPDeck** (called "DeckPad" until v0.6.1, renamed because the name was taken) is a **Stream-Deck-style touch pad** on the Waveshare ESP32-S3-Touch-LCD-7B (7", 1024×600, capacitive touch). The tiles on the display trigger actions on the Windows PC: volume, media keys, any key combination, program launches. The PC sees the board through its native USB port as a **keyboard (incl. media keys) plus a COM port**. All tiles are generated in code from **profiles**; every program gets its own profile with its own keys, which can be arranged freely in the grid.

The project is an **ESP-IDF project** (Arduino-ESP32 runs as a component for `Serial`, LittleFS and ArduinoJson), with no Wi-Fi and no OTA.

> **🟡 Beta (v0.7.0):** The core is finished and runs on the device and on the Windows PC (keyboard, media keys, COM port, editor, icons, free tile positions, quick select). No streaks when saving, no flicker. Calculator, timer, the Windows helper and helper mode are not yet thoroughly tested; file formats and commands may still change before 1.0. Please report bugs as an [issue](https://github.com/JPPeterson-lab/JPDeck/issues).

## 🚀 Installation

**Recommended: web installer** – <https://jppeterson-lab.github.io/JPDeck/>. Open it in Chrome or Edge, connect the board via the **"UART"** port (the "USB" port must be free), click "Install" and choose the port "USB-Enhanced-SERIAL CH343". Afterwards connect the cable of the **"USB"** port to the PC. "Erase device" also deletes saved profiles and icons; without it they are kept (update).

> **Building it yourself** is possible but requires **good knowledge of the ESP32 world** (ESP-IDF, partitions, flash options, drivers): the ESP ecosystem is complex. If you do not need that, use the web installer. The former Arduino IDE variant is gone; the project is fully on ESP-IDF now.

### Building it yourself (for experienced users)
1. Install **ESP-IDF 5.5.x**: `git clone --branch v5.5.5 --recursive https://github.com/espressif/esp-idf.git ~/esp/esp-idf-v5.5.5 && cd ~/esp/esp-idf-v5.5.5 && ./install.sh esp32s3`
2. Connect the board via the **UART port**, then in the project folder:
```bash
bash build.sh flash /dev/cu.usbmodemXXXX     # port of the UART connector (CH343)
```
3. Afterwards connect the **native USB port** to the PC. **While flashing it must not be connected to another computer** (the write aborts otherwise). Windows sets the device up afresh on first connect (own USB id `303A:4DC0`).

More commands: `bash build.sh` (build only), `bash build.sh clean`, pixel clock `PCLK=40 bash build.sh flash …` (default 30 MHz, about 33 frames/s). Release image for the web installer: `bash tools/prepare_release.sh`.

## 📊 Status

| Component | Status |
|---|---|
| 1024×600 RGB display, GT911 touch, LVGL partial rendering, pixel clock 30 MHz | ✅ verified on the device |
| Flash-safe display drive (no streaks when saving) | ✅ verified on the device |
| USB composite: HID keyboard + media keys + COM port (own USB id) | ✅ verified on the PC |
| Tile grid (5×3, up to 6×4), colour change on touch | ✅ verified |
| **Arrange tiles freely in the grid** (gaps allowed, drag in the editor) | ✅ verified |
| **Quick select: "Standard" key and P1–P5** (assignable in the editor, on every profile page) | 🧪 new in v0.6.0 (P1–P5 since v0.6.1), untested |
| Profiles in flash (JSON per profile), selection by roller and arrow keys | ✅ verified |
| **Theme and brightness apply to the whole device** | ✅ verified |
| JPDeck Editor (Web Serial, colours, actions, icons) | ✅ verified on the device |
| Icons on tiles (emoji or own images) | ✅ verified on the device |
| Program launch (`run`) | ✅ verified |
| Time from the PC (set by the editor) | ✅ verified |
| Calculator, kitchen timer/stopwatch | 🧪 untested |
| Automatic profile switch (switch + Windows helper) | 🧪 untested |
| Actions via the PC helper instead of the USB keyboard | 🧪 untested |
| Web installer (GitHub Pages) | ✅ installation verified on the device (update without "erase device") |

## 🎬 Screens

| Screen | Content |
|---|---|
| **Deck** | Time, PC status (green/red), profile name, tile grid, arrows to switch profile, gear |
| **Settings** | Brightness, theme (light/dark), profile roller, version, switches for auto profile switch and "run keys on the PC", calculator and timer |
| **Calculator** | Four basic operations, percent, sign |
| **Timer / stopwatch** | Kitchen timer with time keys (keeps running in the background, the display blinks when it ends), stopwatch with lap times |
| **Boot** | Status line and bar at start-up |

## 🎹 Profiles and tiles

Each profile is a JSON file `/profiles/<id>.json` on the data partition: name, program, grid and the **tiles**. Every tile has a fixed **slot** (`slot`, 0 = top left, row by row). Free slots stay empty, so tiles can sit anywhere in the grid. **Theme (light/dark) and brightness** are not part of the profile, they apply to the whole device. See the JSON example above.

| `type` | `spec` | Effect |
|---|---|---|
| `none` | – | colour change only |
| `media` | `vol_up`, `vol_down`, `mute`, `play_pause`, `next`, `prev`, `stop`, `calculator`, `browser`, `mail` | media / consumer key |
| `keys` | e.g. `ctrl+shift+m` (parts joined by `+`) | key combination; modifiers `ctrl shift alt win altgr`, keys `a-z 0-9 f1-f24 enter esc tab space backspace delete insert home end pageup pagedown up down left right printscreen pause capslock` |
| `text` | any text | typed out (ASCII, PC layout) |
| `run` | `notepad`, `C:\path\x.exe`, `https://…` | starts a program/file/link via Win+R |
| `host` | free | sent as `ACTION <spec>` over the COM port to a PC program |

On first start the device creates two **example profiles**: **Standard** and **Bambu Studio**. Switch with the **roller** in the settings or the **arrow keys** on the deck. Below the tile grid are the **quick keys**: **Standard** jumps back to the profile "standard" from any profile, **P1–P5** jump to a profile you assign in the editor (section "Schnellwahl am Gerät"). They are visible on every profile page; unassigned keys are dimmed, the active profile has a blue outline.

### Creating profiles on the PC (JPDeck Editor)
On the Windows PC open <https://jppeterson-lab.github.io/JPDeck/editor.html> in **Chrome or Edge** (also usable as a [single file](https://raw.githubusercontent.com/JPPeterson-lab/JPDeck/main/tools/editor/jpdeck-editor.html)) and choose **"Mit JPDeck verbinden"** (board connected via "USB"; the COM port must not be open elsewhere). There you can:
- pick a profile or create **Neu** (new) / **Kopie** (copy), set name, program and grid,
- **click** tiles (label, **colour**, **action**, **icon**) and **drag** them: dropping on an empty slot moves, dropping on an occupied tile swaps both,
- **"Auf JPDeck speichern"** (save); the device applies the profile immediately.

On connect the editor also sets the PC's **time** and shows status and device log. Without a device try **"Demo ohne Gerät"**. **Start a program** (`run`): the JPDeck presses Win+R, types the text and confirms with Enter (program name, path or web address).

### Icons on tiles
In the editor **"Symbol wählen"** (emoji) or **"Bild laden …"** (PNG, JPG, SVG), size 32–80 px (with text underneath prefer 48 px or smaller). On save the editor loads new icons onto the device (up to 96 × 96 px, shared by all profiles).

## 🔄 PC helper (Windows)
A small Windows program: download [`JPDeck-Helper.exe`](https://jppeterson-lab.github.io/JPDeck/JPDeck-Helper.exe) and double-click it, nothing to install (.NET Framework 4.8 ships with Windows 10/11). It shows a status window and then keeps running as a tray icon. Windows warns about the unsigned file (SmartScreen): **More info → Run anyway**. It releases the COM port while the editor is in the foreground. Two jobs, both switchable in the device's settings:
- **Automatic profile switch:** picks the profile by the program in the foreground (profile field "Programm", otherwise "Standard").
- **Run keys on the PC:** performs keys, media keys, text and program launches on the PC instead of the board acting as a USB keyboard (fallback).

The helper **only runs when you start it**. The window has two options, both off by default: **Mit Windows starten (im Hintergrund)** (start with Windows, in the background) and **Beenden, wenn das JPDeck getrennt wird** (quit when the JPDeck is unplugged). Closing the window only hides it; **Beenden** (window or right-click on the tray icon) quits the helper.

The helper only sends the name of the foreground program to the JPDeck and, in the second mode, executes what the connected JPDeck sends: only use devices you plugged in yourself.

## 🔌 Hardware

| Part | Details |
|---|---|
| Board | [Waveshare ESP32-S3-Touch-LCD-7B](https://www.waveshare.com/esp32-s3-touch-lcd-7b.htm) – ESP32-S3, 16 MB flash, 8 MB PSRAM (OPI) |
| Display | 7", 1024×600, 16-bit RGB interface |
| Touch | GT911 (I²C), capacitive |
| Backlight | PWM via the I²C IO expander |
| Two USB-C ports | **UART** (CH343): flashing + debug output · **USB** (native, GPIO19/20): link to the PC |

**GPIO19/20 are shared between USB and CAN.** **IO5 of the IO expander** switches them (`0` = USB, confirmed by Waveshare). The JPDeck sets IO5 to USB after the display init (`DECK_USB_SELECT_LEVEL`), so **CAN cannot be used with the JPDeck.**

**USB id:** `303A:4DC0` (own PID). `303A:1001` belongs to the chips' USB-JTAG interface; its Windows driver displaces the keyboard there. For a release, request your own PID (Espressif allocates them free of charge).

### Storage
| Area | Size | Content |
|---|---|---|
| `nvs` | 24 KB | system data |
| `factory` | 4 MB | firmware (about 1 MB), **no OTA** |
| `coredump` | 64 KB | crash dump |
| `storage` (LittleFS) | 11.9 MB | profiles, icons, settings |
| PSRAM (8 MB, 120 MHz) | – | framebuffer, LVGL pool, code and constants |

Project structure: see the German section above. The UI logic (`deck_ui.cpp`) only knows LVGL and profiles; a tile calls `deckActionRun()`, and only `deck_actions.cpp` talks to `usb/`. After every PicoPixel export run `./tools/deploy_ui.sh`.

## 📚 Wiki
Detailed pages (hardware, getting started, profiles, USB and CDC, development): <https://github.com/JPPeterson-lab/JPDeck/wiki>

## 🙏 Credits
See [CREDITS.md](CREDITS.md). GUI: [LVGL](https://lvgl.io). UI design: [PicoPixel](https://picopixel.io), icons: [Bootstrap Icons](https://icons.getbootstrap.com/). Drivers: Waveshare / Espressif.

## 📄 License
[MIT](LICENSE). Individual third-party files keep their licence (Waveshare/Espressif drivers: Apache-2.0, keyboard layouts: LGPL-2.1), see [NOTICE](NOTICE), [CREDITS.md](CREDITS.md) and `LICENSES/`.
