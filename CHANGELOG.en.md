# Changelog

*[Deutsche Version](CHANGELOG.md)*

## v0.6.1 (2026-10-16)

### New
- **Five quick keys instead of three:** **P1–P5** can be assigned freely to profiles in the editor (section "Schnellwahl am Gerät" with five selectors). The CDC command `QUICK` now takes five profile ids (`QUICK <id1> … <id5>`, `-` = free). An existing P1–P3 assignment is kept, P4 and P5 start out free
- New UI from PicoPixel

### Changed
- **UI symbols now come from [Bootstrap Icons](https://icons.getbootstrap.com/)** (MIT) instead of Icons8's Flat Color Icons; credits, README and installer page updated

## v0.6.0 (2026-10-16) – Beta

The project moves from **alpha** to **beta**: the feature set is complete and the core functions are verified on the device and on the Windows PC.

### New
- **Quick keys on the deck screen** (new PicoPixel UI): **Standard** jumps from any profile back to the profile `standard`. **P1–P3** jump to a freely chosen profile. They sit next to the tile grid and are therefore visible and usable on every profile page. Unassigned keys are dimmed and do nothing, the active profile has a blue outline
- **Assignment in the editor:** section "Schnellwahl am Gerät" with one selector each for P1–P3 (takes effect at once). Stored device-wide in `config.json` (`quick`), returned by `LIST`; new CDC command `QUICK <id1> <id2> <id3>` (`-` = free). If a profile is deleted the key becomes free
- **Windows helper as a proper program:** `DeckPad-Helper.exe` (C#, WinForms, .NET Framework 4.8, about 80 KB) replaces the PowerShell script. Status window, tray icon, single instance. Options (both **off** by default): *Mit Windows starten (im Hintergrund)* (per-user autostart, never running permanently without consent) and *Beenden, wenn das DeckPad getrennt wird*. Source in `tools/helper-app/`, build with `tools/build_helper.sh`. The file is unsigned (SmartScreen notice)

### Changed
- New UI exports (font size 62 dropped, 92 added, icons for the quick keys)

## v0.5.0 (2026-10-15)

### New
- **Arrange tiles freely in the grid:** every tile has a fixed position (`slot`), gaps are allowed. In the editor **drag** tiles: dropping on an empty slot moves, dropping on an occupied tile swaps both. When the grid size changes tiles keep their row and column; whatever no longer fits is removed (after a confirmation). Older profiles without `slot` load unchanged (order)
- **New UI from PicoPixel** (different colours, containers moved, larger profile roller); the extra box on the settings screen now lines up with the containers

### Changed
- **Theme (light/dark) and brightness apply to the whole device**, no longer per profile. Before, the switch only changed the theme of the currently active profile and new profiles always started light. Both values now live in `config.json` and are saved with a delay (no flash write on every adjustment). On update the device adopts theme and brightness of the active profile. The two controls are gone from the editor, the `settings` entries from the profile files
- **Fully moved to ESP-IDF, Arduino IDE dropped:** the IDF project now sits in the root folder (`bash build.sh`, `main/main.cpp`); `platforms/`, `Deckpad.ino`, `build_opt.h` and the Arduino USB layer are removed. Arduino-ESP32 stays as a component for `Serial`, LittleFS and ArduinoJson
- The P4 board will not be used for the DeckPad (it goes into a weather station project); the DeckPad stays on the ESP32-S3

## v0.4.0 (2026-10-13)

### New
- **ESP-IDF build (`platforms/idf/`) as the main path:** same source code, but with its own configuration instead of the precompiled Arduino core. Display drive flash-safe (`LCD_RGB_ISR_IRAM_SAFE`, `GDMA_ISR_IRAM_SAFE`), PSRAM 120 MHz, code in PSRAM, 64 KB data cache. **The streaks when saving no longer occur**, write times drop (29–93 ms instead of 68–199 ms). Build and flash: `platforms/idf/build.sh`
- **Web installer** (`docs/index.html`, GitHub Pages): one click instead of a development environment, using [esp-web-tools](https://esp.huhn.me/). The firmware image (bootloader, partition table and program in one file) lives in `docs/firmware/`; `tools/prepare_release.sh` builds it and writes `manifest.json`. The editor is now at `…/editor.html`
- **Pixel clock 30 MHz** (was 15 MHz): about 33 instead of 16 frames/s, the flicker is gone. Adjustable (`PCLK=40 bash build.sh`, `DECK_PIXEL_CLOCK_HZ`)
- **Own USB layer for IDF** (`src/usb/usb_device_idf.cpp`, `esp_tinyusb`): HID keyboard + media keys + COM port, same interface as the Arduino build
- **Measurements in the log:** per action wait time and send duration (`[Aktion] …: Warten x ms, Senden y ms`), longest UI round in the `[Diag]` line

### Changed
- **Own USB product id `303A:4DC0`** (both builds). 303A:1001 belongs to the chips' USB-JTAG interface; its Windows driver displaced the keyboard under IDF (interface MI_02 was set up as WinUSB instead of HID). Windows sets the device up afresh on first connect (new COM number). The helper looks for the new id first
- **Program start (`run`) and typing text much faster:** HID poll interval 1 ms (was 10 ms), no extra pauses between characters: about 3 ms instead of 13–15 ms per character. A long Chrome command takes 1.2 s instead of 2.2 s (0.7 s of that is the wait for the Run dialog)

### Note
- **Flashing:** when flashing via the UART port, the cable of the native USB port must not be connected to another computer at the same time (otherwise the write aborts)

## v0.3.3 (2026-10-13)

### New
- **Panel restart:** after every flash write the device also restarts the display drive (`esp_lcd_rgb_panel_restart`, per Espressif's documentation against a permanent shift when the LCD controller and DMA get out of sync) and redraws the screen. Plus COM command `PANEL` and an editor button **"Panel neu starten"** for testing. Whether this removes the streaks is not yet confirmed

## v0.3.2 (2026-10-13)

### New
- **Redraw display:** button in the editor and COM command `REDRAW`. Redraws the whole display. If a streak is gone afterwards, only the image content was disturbed; if it stays, the display drive itself is at fault. Used to narrow down the streaks at the left edge
- **Automatic redraw after flash writes** (saving a profile, uploading an icon, saving the selection): the RGB driver in the precompiled core is not IRAM-safe (`CONFIG_LCD_RGB_ISR_IRAM_SAFE` not set), so the drive pauses briefly during writes. Afterwards the whole screen is redrawn once; in the log as `[UI] Vollbild neu gezeichnet`

## v0.3.1 (2026-10-13)

### New
- **"Tasten am PC ausführen" (run keys on the PC):** second switch on the settings screen (default: off). Then the board does not act as a USB keyboard but sends every action as `DO <type> <spec>` over the COM port, and the **Windows helper** performs keys, media keys, text and program launches (`SendInput`, `Start-Process`). Meant for boards without a USB keyboard (e.g. the planned ESP32-P4 board) and as a fallback. Without the helper nothing happens (note in the log and on the display)
- The helper now listens to the device every 40 ms (previously only 0.6 s); the foreground window is still checked every 0.6 s

### Fixed
- **Shortcuts with Z or Y were swapped on German keyboards:** letters and digits were sent as the *physical* key of the US layout; on a German layout the Z position is the Y (`ctrl+z` arrived as Ctrl+Y). Letters and digits now follow the PC's layout (`DECK_KEY_LAYOUT`, default German). Affects e.g. the example profile "Bambu Studio" (undo). Ctrl+Shift+M, Ctrl+C/V, Ctrl+S were not affected. With a US-layout PC set `DECK_KEY_LAYOUT` to `KeyboardLayout_en_US`

## v0.3.0 (2026-10-13)

### New
- **Icons on the tiles:** symbols (emoji picker) or your own images (PNG, JPG, SVG …), converted to 32–80 px in the editor and loaded onto the device over the COM port (RGB565 + alpha, `/icons/<id>.bin`, up to 96 × 96 px). Several profiles can share an icon. The quick-pick in the editor brings matching symbols. COM commands `ICONBEGIN`, `ICONDATA`, `ICONEND`, `ICONLIST`, `ICONGET`, `ICONDEL`
- **Automatic profile switch (switchable):** switch "Automatischer Profilwechsel" on the settings screen (default: off). A small **Windows helper** (`tools/helper/deckpad-helper.ps1`, no installation) reports the foreground program; the DeckPad picks the profile whose "Programm" field matches (e.g. `bambu-studio.exe`), otherwise "Standard". The settings screen shows whether the helper is detected. COM command `ACTIVE <program.exe>`
- **Editor:** a **"Trennen"** (disconnect) button releases the COM port (the helper also releases it by itself while the editor is in the foreground)
- **Status display:** "PC verbunden" has a green background, "Kein PC" a red one

## v0.2.1 (2026-10-13)

### Changed
- **Profile selection is saved with a delay:** every profile switch used to write to flash immediately. Flash writes briefly stop the cache (and with it the display's PSRAM access) and wear the flash. The selection is now only written once nothing was switched for 8 s (`[Flash]` line in the log). Possible cause of the streaks after several profile switches, not yet confirmed

### New
- **More measurements in the log:** duration of the tile rebuild on every profile switch, and individual slow flushes (longer than 20 ms) with size and position of the area

## v0.2.0 (2026-10-13)

### New
- **DeckPad Editor** (`tools/editor/deckpad-editor.html`): create, edit, copy, delete and activate profiles on the **Windows PC** in the browser (Chrome/Edge, Web Serial). Click tiles, set label, **colour** (picker and palette), **action** (media, key combination with modifier boxes and key capture, start a program, text, host message) and quick-pick templates. Back up a profile as a file, demo mode without a device. Sets the PC's **time** on connect. Shows status and device log
- **Editor online:** available via GitHub Pages (<https://jppeterson-lab.github.io/DeckPad/editor.html>), nothing to install. `tools/deploy_editor.sh` copies the file to `docs/index.html`
- **Editor recognises the wrong port:** the chooser only lists Espressif devices (VID 0x303A), i.e. the native USB port; the UART port (CH343) is recognised and explained. "Alle Ports anzeigen" (show all ports) removes the filter
- **Action `run`:** starts a program, file or link via Win+R (program name, path, web address). The device presses Win+R, types the text and confirms with Enter
- **COM commands:** `VER`, `LIST`, `GET <id>`, `PUT <id> <json>`, `DEL <id>`, `USE <id>`, `TIME <unix seconds>` (in addition to `PING`, `STATUS`, `LOG`). The device applies changes immediately (roller, grid, colours)
- **Layers:** the USB layer only knows the channel; commands are registered by `app/host_commands.cpp`

### Changed
- COM line length from 256 to 6144 bytes, CDC task stack to 8 KB (a profile as JSON needs up to about 4 KB)

## v0.1.1 (2026-10-13)

### New
- **Diagnostic log:** ring buffer with the last 64 events (start-up incl. reset reason and display settings, USB events, key presses, profile switches, **flash writes with duration**). Every line also goes to the UART port
- **COM commands `STATUS` and `LOG`:** `STATUS` returns one line with version, uptime, free and minimum memory (SRAM, PSRAM) and the flush times; `LOG` prints the ring buffer
- **Flush measurement:** count, average and maximum duration of copying into the PSRAM framebuffer; a `[Diag]` line on the UART port every 10 s. Purpose: narrow down streaks and torn lines (PSRAM bandwidth) with measurements instead of guesses

## v0.1.0 (2026-10-13)

First skeleton. The code compiles; a test on the device is still pending.

### New
- **Project:** standalone project (drivers for RGB panel, GT911, I²C, IO expander, LVGL partial rendering, `build_opt.h`)
- **Tile grid:** tiles are created in code inside the `ContainerGrid` placeholder of the PicoPixel design. Default 5 × 3, up to 6 × 4 per profile. On touch the tile changes colour immediately and fades back within 350 ms
- **USB to the PC:** native USB port as a composite device with HID keyboard, consumer/media keys and a virtual COM port. `Serial` stays on the USB-C UART port (flashing and debug)
- **USB/CAN switch:** IO5 of the IO expander is set explicitly to USB after the display init (the driver would have left it on CAN)
- **CDC:** line-based command reader with a command table; for now only `PING` is answered with `PONG`
- **Actions:** `media`, `keys` (e.g. `ctrl+shift+m`), `text`, `host`; execution runs in its own task so LVGL never waits for USB
- **Profiles:** one JSON file per program on the LittleFS partition with its own tiles and settings (theme, brightness, grid). Example profiles "Standard" and "Bambu Studio" on first start. Selected via the roller in the settings and the arrow keys on the deck
- **Per-profile settings:** theme and brightness are saved to the active profile as you change them (brightness on release, to spare the flash)
- **Calculator**, opened via the calculator icon in the settings
- **Kitchen timer and stopwatch:** the timer keeps running in the background; when it ends the display blinks (screen and backlight) on any screen until it is tapped (after one minute at the latest). Stopwatch with lap times
- **Partition table without OTA:** 4 MB factory app, 24 KB NVS, 64 KB coredump, 11.9 MB LittleFS
- **Tools:** `tools/deploy_ui.sh` (prepare and check the PicoPixel export), `tools/bump_version.sh`
- **Docs:** README, changelog and devlog in two languages, wiki

### Changed compared to the PicoPixel design
- The 15 static tile containers on the deck screen were replaced by **one** placeholder container `ContainerGrid` (27, 87, 950 × 384). The old containers 1–6 and 9–17 still have to be deleted in PicoPixel; `deploy_ui.sh` reports them as long as they exist

### Known
- The connection event on the device is not yet confirmed (the polarity of IO5, `0` = USB, is confirmed by the Waveshare documentation)
- The time shows `--:--` until a later step sets it from the PC
