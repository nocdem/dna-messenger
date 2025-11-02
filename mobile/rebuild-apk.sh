#!/bin/bash
#
# Quick APK rebuild script
#

set -e

cd "$(dirname "$0")"

echo "Cleaning..."
./gradlew clean

echo "Building debug APK..."
./gradlew :androidApp:assembleDebug

echo ""
echo "✅ APK built successfully!"
echo ""
echo "Location: androidApp/build/outputs/apk/debug/androidApp-debug.apk"
echo ""
echo "Install with:"
echo "  adb install -r androidApp/build/outputs/apk/debug/androidApp-debug.apk"
echo ""
