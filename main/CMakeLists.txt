# Die Anwendung liegt in src/ (Wurzel des Repos); main/ enthaelt nur den Einstieg (main.cpp).
get_filename_component(DECK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
file(GLOB_RECURSE DECK_SRCS "${DECK_ROOT}/src/*.c" "${DECK_ROOT}/src/*.cpp")

idf_component_register(
    SRCS "main.cpp" ${DECK_SRCS}
    INCLUDE_DIRS "." "${DECK_ROOT}" "${DECK_ROOT}/src"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)
# Pixeltakt in MHz (Bildrate = Takt / (1386 x 661)): idf.py -DDECK_PIXEL_CLOCK_MHZ=40 build
if(NOT DEFINED DECK_PIXEL_CLOCK_MHZ)
    set(DECK_PIXEL_CLOCK_MHZ 30)
endif()
target_compile_definitions(${COMPONENT_LIB} PRIVATE DECK_PIXEL_CLOCK_HZ=${DECK_PIXEL_CLOCK_MHZ}000000)

# Die Arduino-Komponente uebersetzt ihre eigenen USB-Klassen (USBCDC, USBHID ...), sobald TinyUSB aktiv ist. Sie passen nicht zur
# IDF-TinyUSB-Schicht, die wir selbst stellen (src/usb/usb_device_idf.cpp): diese Quellen aus dem Build nehmen.
if(TARGET __idf_espressif__arduino-esp32)
    get_target_property(_arduino_srcs __idf_espressif__arduino-esp32 SOURCES)
    list(FILTER _arduino_srcs EXCLUDE REGEX "cores/esp32/(USB|USBCDC|USBMSC|USBHost|FirmwareMSC|firmware_msc_fat|esp32-hal-tinyusb)\\.(c|cpp)$")
    set_property(TARGET __idf_espressif__arduino-esp32 PROPERTY SOURCES "${_arduino_srcs}")
endif()
