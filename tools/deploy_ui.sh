#!/bin/bash
# PicoPixel-UI-Export für die Arduino IDE / arduino-cli vorbereiten.
# PicoPixel Exportziel: src/ui/   (der ESP-IDF-Build uebersetzt src/ rekursiv)
# Aufruf nach jedem Export:  bash tools/deploy_ui.sh
SRC="$(cd "$(dirname "$0")/.." && pwd)/src/ui"
if [ ! -d "$SRC" ]; then echo "FEHLER: src/ui/ nicht gefunden."; exit 1; fi

# PicoPixel kann "lvgl/lvgl.h" erzeugen – Arduino kennt nur <lvgl.h>
find "$SRC" \( -name "*.c" -o -name "*.h" \) | while read -r f; do
  if grep -q '"lvgl/lvgl.h"' "$f"; then
    sed -i '' 's|#include "lvgl/lvgl.h"|#include <lvgl.h>|g' "$f"
    echo "  lvgl-Include gepatcht: $(basename "$f")"
  fi
done

# images/*.c werden von images.c per #include eingebunden UND von Arduino direkt kompiliert
# -> doppelte Definitionen; Loesung: zu .inc umbenennen
if [ -d "$SRC/images" ]; then
  for f in "$SRC/images"/*.c; do
    [ -f "$f" ] || continue
    mv "$f" "${f%.c}.inc"
    echo "  Image umbenannt: $(basename "$f") -> $(basename "${f%.c}").inc"
  done
  if [ -f "$SRC/images.c" ]; then
    sed -i '' 's|#include "images/\(.*\)\.c"|#include "src/ui/images/\1.inc"|g' "$SRC/images.c"
    sed -i '' 's|#include "images/\(.*\)\.inc"|#include "src/ui/images/\1.inc"|g' "$SRC/images.c"
    echo "  images.c: Include-Pfade auf src/ui/images/*.inc gesetzt"
  fi
fi

# screens.c: PicoPixel exportiert Farben als lv_color_hex(0xffRRGGBB)
if [ -f "$SRC/screens.c" ]; then
  sed -i '' 's/lv_color_hex(0xff/lv_color_hex(0x/g' "$SRC/screens.c"
  echo "  screens.c: lv_color_hex 0xff-Prefix entfernt"
fi

# ui.c: FADE_IN-Animation -> NONE (Absturz-Fix, spart Rechenzeit)
if [ -f "$SRC/ui.c" ]; then
  sed -i '' 's|lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false)|lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false)|g' "$SRC/ui.c"
  echo "  ui.c: FADE_IN-Animation -> NONE"
fi

# Sonderzeichen, die die eingebauten Montserrat-Schriften nicht haben
if [ -f "$SRC/screens.c" ] && grep -q 'mg/m²' "$SRC/screens.c"; then
  sed -i '' 's|mg/m²|ug/m3|g' "$SRC/screens.c"
  echo "  screens.c: mg/m² -> ug/m3"
fi

# Pruefen, ob alle vom Code verdrahteten Objekte noch existieren
echo; echo "Verifikation screens.h - erwartete Objekte:"
EXPECTED=(
  "screenboot" "labelstatus" "barwifi"
  "screen_1" "containergrid" "labeltime" "labelprofil" "labelconnectionpc"
  "labelbuttonbackward" "labelbuttonforward" "buttonsetting"
  "screensetting" "rollerprofile" "labellightchangevalue" "labelswitchtheme"
  "labelversion" "labelbuttonhome" "buttoncalc" "buttonclock"
)
MISSING=0
for obj in "${EXPECTED[@]}"; do
  if ! grep -q "\b${obj}\b" "$SRC/screens.h"; then
    echo "  WARNUNG: '$obj' nicht in screens.h gefunden - Objekt umbenannt oder geloescht?"
    MISSING=$((MISSING+1))
  fi
done
# Die alten statischen Kachel-Container (Container 1-6, 9-17) duerfen nicht mehr existieren
if grep -Eq "\bcontainer_(1|2|3|4|5|6|9|1[0-7])\b" "$SRC/screens.h"; then
  echo "  WARNUNG: alte statische Kachel-Container in PicoPixel gefunden - die Kacheln entstehen jetzt im Code"
  echo "           innerhalb von 'ContainerGrid'. Die Container 1-6 und 9-17 in PicoPixel loeschen."
  MISSING=$((MISSING+1))
fi
if [ $MISSING -eq 0 ]; then echo "  Alles in Ordnung."; else echo; echo "  !! $MISSING Hinweis(e) - siehe oben !!"; fi
echo; echo "Fertig. src/ui/ ist bereit fuer den ESP-IDF-Build."
