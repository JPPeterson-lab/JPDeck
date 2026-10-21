#!/usr/bin/env bash
# Baut den Windows-Helfer (DeckPad-Helper.exe) auf dem Mac und legt ihn nach docs/ (Download fuer die Webseite).
# Voraussetzung: .NET 8 SDK unter ~/.dotnet (https://dot.net/v1/dotnet-install.sh). Die exe ist nur unter Windows lauffaehig.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export DOTNET_ROOT="$HOME/.dotnet" PATH="$HOME/.dotnet:$PATH" DOTNET_CLI_TELEMETRY_OPTOUT=1
dotnet build "$ROOT/tools/helper-app/DeckPadHelper.csproj" -c Release
cp "$ROOT/tools/helper-app/bin/Release/net48/DeckPad-Helper.exe" "$ROOT/docs/DeckPad-Helper.exe"
ls -l "$ROOT/docs/DeckPad-Helper.exe"
