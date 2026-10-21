#!/bin/bash
# Version hochzählen: bash tools/bump_version.sh 0.1.0 0.2.0
cd "$(dirname "$0")/.." || exit 1
OLD=$1; NEW=$2
if [ -z "$OLD" ] || [ -z "$NEW" ]; then echo "Verwendung: bash tools/bump_version.sh <alt> <neu>"; exit 1; fi
sed -i '' "s/FIRMWARE_VERSION \"$OLD\"/FIRMWARE_VERSION \"$NEW\"/" src/app/app_config.h
sed -i '' "s/Version-v$OLD-blue/Version-v$NEW-blue/g; s/Alpha (v$OLD)/Alpha (v$NEW)/g; s/Beta (v$OLD)/Beta (v$NEW)/g" README.md
grep "FIRMWARE_VERSION" src/app/app_config.h
echo "Fertig. Danach CHANGELOG.md / CHANGELOG.en.md ergänzen, kompilieren, committen."
