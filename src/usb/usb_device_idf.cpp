// USB-Geraet fuer den ESP-IDF-Build (esp_tinyusb): HID-Tastatur + Consumer (Media) + CDC (COM-Port) als Composite-Geraet.
// Schnittstelle: siehe usb_device.h.
#include "usb_device.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_cdc_acm.h"
#include "class/hid/hid_device.h"
#include "keyboard_layouts.h"
#include "../hardware/io_extension.h"
#include "../app/app_config.h"
#include "../app/diag_log.h"

#if DECK_KEY_LAYOUT_DE
#define DECK_LAYOUT deckLayoutDE
#else
#define DECK_LAYOUT deckLayoutUS
#endif

DeckSerialIdf DeckSerial;
static volatile bool s_hostUp = false;
static volatile bool s_dtr    = false;
static StreamBufferHandle_t s_rx = nullptr;

// ── Deskriptoren ──────────────────────────────────────────────────────────────
enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_HID, ITF_NUM_TOTAL };
#define EPNUM_CDC_NOTIF 0x81
#define EPNUM_CDC_OUT   0x02
#define EPNUM_CDC_IN    0x82
#define EPNUM_HID       0x83
#define REPORT_ID_KEYBOARD 1
#define REPORT_ID_CONSUMER 2

static const uint8_t s_hidReport[] = {
  TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD)),
  TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER)),
};

#define CFG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)
static const uint8_t s_cfgDesc[] = {
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CFG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
  // Abfrageintervall 1 ms (wie die Arduino-Bibliothek): bei 10 ms wartet jeder Bericht bis zu 10 ms, ein Text dauert dann ewig
  TUD_HID_DESCRIPTOR(ITF_NUM_HID, 5, HID_ITF_PROTOCOL_NONE, sizeof(s_hidReport), EPNUM_HID, 16, 1),
};

// VID/PID siehe app_config.h (eigene PID: 0x1001 gehoert der USB-JTAG-Schnittstelle und kollidiert unter Windows auf MI_02)
static const tusb_desc_device_t s_devDesc = {
  .bLength = sizeof(tusb_desc_device_t), .bDescriptorType = TUSB_DESC_DEVICE, .bcdUSB = 0x0200,
  .bDeviceClass = TUSB_CLASS_MISC, .bDeviceSubClass = MISC_SUBCLASS_COMMON, .bDeviceProtocol = MISC_PROTOCOL_IAD,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = DECK_USB_VID, .idProduct = DECK_USB_PID, .bcdDevice = 0x0100,
  .iManufacturer = 1, .iProduct = 2, .iSerialNumber = 3, .bNumConfigurations = 1,
};

static const char* s_strings[] = {
  (const char[]){ 0x09, 0x04 },     // 0: Sprache (Englisch)
  DECK_USB_MANUFACTURER,            // 1
  DECK_USB_PRODUCT,                 // 2
  "DECKPAD-0001",                   // 3: feste Seriennummer, damit Windows immer dieselbe COM-Nummer vergibt
  "JPDeck COM",                    // 4: CDC
  "JPDeck Tasten",                 // 5: HID
};

extern "C" {
uint8_t const* tud_hid_descriptor_report_cb(uint8_t) { return s_hidReport; }
uint16_t tud_hid_get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t*, uint16_t) { return 0; }
void tud_hid_set_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t const*, uint16_t) {}   // LEDs der Tastatur: nicht benoetigt
}

// ── Ereignisse ────────────────────────────────────────────────────────────────
static void onUsbEvent(tinyusb_event_t* e, void*)
{
  switch (e->id) {
    case TINYUSB_EVENT_ATTACHED: s_hostUp = true;  logDiag("[USB] Ereignis: gestartet (PC hat das Geraet erkannt)"); break;
    case TINYUSB_EVENT_DETACHED: s_hostUp = false; logDiag("[USB] Ereignis: gestoppt (Kabel ab)"); break;
#ifdef CONFIG_TINYUSB_SUSPEND_CALLBACK
    case TINYUSB_EVENT_SUSPENDED: s_hostUp = false; logDiag("[USB] Ereignis: Standby"); break;
#endif
#ifdef CONFIG_TINYUSB_RESUME_CALLBACK
    case TINYUSB_EVENT_RESUMED: s_hostUp = true; logDiag("[USB] Ereignis: aus dem Standby"); break;
#endif
    default: break;
  }
}

static void cdcRx(int itf, cdcacm_event_t*)
{
  uint8_t buf[64]; size_t n = 0;
  while (tinyusb_cdcacm_read((tinyusb_cdcacm_itf_t)itf, buf, sizeof(buf), &n) == ESP_OK && n > 0)
    xStreamBufferSend(s_rx, buf, n, 0);
}
static void cdcLine(int, cdcacm_event_t* e) { s_dtr = e->line_state_changed_data.dtr; }

// ── CDC als Stream (wie USBCDC im Arduino-Build) ──────────────────────────────
int DeckSerialIdf::available() { return s_rx ? (int)xStreamBufferBytesAvailable(s_rx) : 0; }
int DeckSerialIdf::read()
{
  uint8_t c;
  return (s_rx && xStreamBufferReceive(s_rx, &c, 1, 0) == 1) ? c : -1;
}
DeckSerialIdf::operator bool() const { return s_dtr; }
size_t DeckSerialIdf::print(const char* s)
{
  const size_t n = strlen(s);
  size_t sent = 0;
  const TickType_t t0 = xTaskGetTickCount();
  while (sent < n && s_dtr) {
    size_t q = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (const uint8_t*)s + sent, n - sent);
    sent += q;
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, pdMS_TO_TICKS(100));
    if (!q) vTaskDelay(1);
    if (xTaskGetTickCount() - t0 > pdMS_TO_TICKS(2000)) break;       // Gegenseite liest nicht: nicht ewig blockieren
  }
  return sent;
}

// ── Start ─────────────────────────────────────────────────────────────────────
void usbDeviceInit()
{
  // Die nativen USB-Pins GPIO19/20 teilt sich das Board mit dem CAN-Transceiver (IO5 des IO-Expanders).
  IO_EXTENSION_Output(IO_EXTENSION_IO_5, DECK_USB_SELECT_LEVEL);
  logDiag("[USB] IO5 = %d (USB-Pfad aktiv)", DECK_USB_SELECT_LEVEL);

  s_rx = xStreamBufferCreate(8192, 1);
  tinyusb_config_t cfg = TINYUSB_DEFAULT_CONFIG(onUsbEvent);
  cfg.task.xCoreID = 0;                                              // LVGL laeuft auf Core 1
  cfg.descriptor.device = &s_devDesc;
  cfg.descriptor.string = s_strings;
  cfg.descriptor.string_count = sizeof(s_strings) / sizeof(s_strings[0]);
  cfg.descriptor.full_speed_config = s_cfgDesc;
  esp_err_t err = tinyusb_driver_install(&cfg);
  if (err != ESP_OK) { logDiag("[USB] tinyusb_driver_install FEHLER %d", (int)err); return; }

  tinyusb_config_cdcacm_t acm = {};
  acm.cdc_port = TINYUSB_CDC_ACM_0;
  acm.callback_rx = cdcRx;
  acm.callback_line_state_changed = cdcLine;
  err = tinyusb_cdcacm_init(&acm);
  logDiag(err == ESP_OK ? "[USB] HID-Tastatur + Consumer + CDC gestartet" : "[USB] CDC-Start FEHLER %d", (int)err);
}

bool usbHostConnected() { return s_hostUp; }

// ── HID ───────────────────────────────────────────────────────────────────────
static bool hidWait()
{
  for (int i = 0; i < 100; i++) { if (tud_hid_ready()) return true; vTaskDelay(pdMS_TO_TICKS(2)); }
  return false;
}
static void sendKbd(uint8_t mod, uint8_t keys[6]) { if (hidWait()) tud_hid_keyboard_report(REPORT_ID_KEYBOARD, mod, keys); }

// Ein ASCII-Zeichen laut Layout in Modifier + Tastencode zerlegen (wie USBHIDKeyboard::press)
static bool layoutKey(uint8_t c, uint8_t& mod, uint8_t& code)
{
  uint8_t e = DECK_LAYOUT[c & 0x7F];
  if (!e) return false;
  if (e & DECK_KL_SHIFT) { mod |= 0x02; e &= ~DECK_KL_SHIFT; }
  if ((e & DECK_KL_ALT_GR) == DECK_KL_ALT_GR) { mod |= 0x40; e &= ~DECK_KL_ALT_GR; }
  if (e == DECK_KL_ISO_REPLACEMENT) e = DECK_KL_ISO_KEY;
  code = e;
  return true;
}

void usbKeyChord(const uint16_t* keys, uint8_t count)
{
  uint8_t mod = 0, kc[6] = {0}, n = 0;
  for (uint8_t i = 0; i < count; i++) {
    const uint16_t k = keys[i];
    if (k & 0x100) { uint8_t code; if (layoutKey((uint8_t)(k & 0xFF), mod, code) && n < 6) kc[n++] = code; }
    else if (k >= 0xE0 && k < 0xE8) mod |= (uint8_t)(1u << (k - 0xE0));
    else if (k && n < 6) kc[n++] = (uint8_t)k;
  }
  sendKbd(mod, kc);
  vTaskDelay(pdMS_TO_TICKS(25));                // kurz halten, damit Windows die Kombination sicher erkennt
  uint8_t none[6] = {0};
  sendKbd(0, none);
}

void usbConsumerTap(uint16_t usage)
{
  if (hidWait()) tud_hid_report(REPORT_ID_CONSUMER, &usage, sizeof(usage));
  vTaskDelay(pdMS_TO_TICKS(25));
  uint16_t zero = 0;
  if (hidWait()) tud_hid_report(REPORT_ID_CONSUMER, &zero, sizeof(zero));
}

void usbTypeText(const char* text)
{
  if (!text) return;
  for (const char* p = text; *p; p++) {
    uint8_t mod = 0, code = 0, kc[6] = {0};
    if (!layoutKey((uint8_t)*p, mod, code)) continue;       // nur ASCII
    kc[0] = code;
    sendKbd(mod, kc);                              // sendKbd wartet selbst, bis der vorige Bericht abgeholt ist
    uint8_t none[6] = {0};
    sendKbd(0, none);
  }
}
