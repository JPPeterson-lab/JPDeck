#!/bin/bash
# Veroeffentlicht Editor und Helfer ueber GitHub Pages: kopiert den Editor nach docs/editor.html (den Windows-Helfer baut tools/build_helper.sh).
# Danach committen und pushen; Pages liefert https://jppeterson-lab.github.io/JPDeck/editor.html
cd "$(dirname "$0")/.." || exit 1
cp tools/editor/jpdeck-editor.html docs/editor.html && echo "docs/editor.html aktualisiert"
