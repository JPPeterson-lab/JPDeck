# JPDeck: ESP-IDF-Projekt (Arduino-ESP32 als Komponente fuer Serial, LittleFS, ArduinoJson).
# Eigene sdkconfig: flash-sicherer Display-Treiber, PSRAM 120 MHz, groesserer Daten-Cache.
# Bauen:  bash build.sh   (ESP-IDF 5.5.x muss installiert sein, siehe README)
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# LVGL sucht seine lv_conf.h im Include-Pfad: die Projektkopie liegt in conf/
include_directories("${CMAKE_CURRENT_LIST_DIR}/conf")
# Fremdkomponenten der Arduino-Komponente (esp-modbus u. a.) haben Warnungen, die in IDF 5.5 als Fehler gelten: nur Warnung
idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)
# esp-modbus hängt ein eigenes -Werror an: nur gezielte -Wno-error=... schlagen es
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=stringop-truncation;-Wno-error=format-truncation;-Wno-error=restrict;-Wno-error=array-bounds;-Wno-error=maybe-uninitialized;-Wno-error=address-of-packed-member" APPEND)
project(jpdeck)
