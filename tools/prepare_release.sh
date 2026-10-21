#!/bin/bash
# Baut den ESP-IDF-Build und legt das Firmware-Image fuer den Web-Installer ab:
#   docs/firmware/deckpad-merged.bin   Bootloader + Partitionstabelle + Programm in einer Datei (ab Offset 0)
#   docs/manifest.json, docs/version.json
# Ablauf: Version hochzaehlen (tools/bump_version.sh), CHANGELOG ergaenzen, dann dieses Skript, danach committen und pushen.
# Optional:  PCLK=40 bash tools/prepare_release.sh   (Pixeltakt in MHz, Standard 30)
set -e
cd "$(dirname "$0")/.."
VER=$(grep -o 'FIRMWARE_VERSION "[^"]*"' src/app/app_config.h | cut -d'"' -f2)
source ~/esp/esp-idf-v5.5.5/export.sh > /dev/null 2>&1 || { echo "FEHLER: ESP-IDF 5.5.5 nicht gefunden (~/esp/esp-idf-v5.5.5)"; exit 1; }

[ -f sdkconfig ] || idf.py set-target esp32s3
idf.py -DDECK_PIXEL_CLOCK_MHZ="${PCLK:-30}" build
idf.py merge-bin -o deckpad-merged.bin   # Pfad gilt relativ zum Build-Ordner

IMG=build/deckpad-merged.bin
APP=build/deckpad.bin
[ -f "$IMG" ] || { echo "FEHLER: $IMG fehlt"; exit 1; }
# Die Versionsnummer muss im Programm stehen (sonst wurde nach dem Hochzaehlen nicht neu gebaut)
strings -a "$APP" | grep -qF "Start v$VER" || { echo "FEHLER: Version $VER steht nicht im Programm"; exit 1; }
# Das zusammengefuehrte Image muss die Teile an den richtigen Stellen enthalten
python3 - "$IMG" build/bootloader/bootloader.bin build/partition_table/partition-table.bin "$APP" <<'PY'
import sys
img=open(sys.argv[1],'rb').read()
for path,off in ((sys.argv[2],0x0),(sys.argv[3],0x8000),(sys.argv[4],0x10000)):
    part=open(path,'rb').read()
    # Bytes 2-3 des Bootloader-Kopfes (Flash-Modus/-Groesse) setzt merge-bin neu, daher nur ab Byte 8 vergleichen
    skip = 8 if off == 0 else 0
    assert img[off+skip:off+len(part)] == part[skip:], f"Teil bei 0x{off:x} stimmt nicht"
print("Image geprueft:", len(img), "Bytes")
PY
mkdir -p docs/firmware
cp "$IMG" docs/firmware/deckpad-merged.bin
python3 - "$VER" <<'PY'
import json, sys
ver = sys.argv[1]
json.dump({
  "name": "DeckPad", "version": ver,
  "funding_url": "https://github.com/JPPeterson-lab/DeckPad",
  "new_install_prompt_erase": True,
  "builds": [{ "chipFamily": "ESP32-S3", "improv": False,
               "parts": [{ "path": "firmware/deckpad-merged.bin", "offset": 0 }] }]
}, open("docs/manifest.json", "w"), indent=2)
json.dump({"version": ver}, open("docs/version.json", "w"))
PY
sed -i '' "s|<span class=\"badge\">v[0-9.]*</span>|<span class=\"badge\">v$VER</span>|" docs/index.html 2>/dev/null || true
ls -la docs/firmware
echo "Version $VER. Jetzt: git add -A && git commit && git push"
