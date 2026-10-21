// Taschenrechner (1024×600): der Button "buttoncalc" auf dem Einstellungs-Screen öffnet ihn.
// Der Screen wird erst beim Öffnen angelegt und beim Schließen wieder freigegeben, damit er im Normalbetrieb
// keinen LVGL-Speicher belegt. Alles läuft im LVGL-Task (Callbacks), kein Zugriff aus anderen Tasks.
#include <Arduino.h>
#include "diag_log.h"
#include "../display/lvgl_port.h"
#include "lv_lock.h"
#include <math.h>
#include "tools.h"
#include "navigation.h"
#include "../ui/screens.h"
#include "../ui/fonts/fonts.h"
#include "app_fonts.h"

static lv_obj_t* rcScreen = nullptr;
static lv_obj_t* rcWert   = nullptr;   // große Anzeige
static lv_obj_t* rcFormel = nullptr;   // kleine Zeile darüber
static String    rcEingabe = "0";      // aktuelle Eingabe, '.' als Dezimaltrenner (Anzeige mit ',')
static String    rcFormelText = "";
static double    rcAkku = 0;
static char      rcOp = 0;             // '+', '-', '*', '/' oder 0
static bool      rcNeu = true;         // nächste Ziffer beginnt eine neue Eingabe
static bool      rcFehler = false;

// Gemeinsame Styles: ein Satz für alle Tasten statt Einzel-Styles pro Objekt (spart LVGL-Speicher)
static lv_style_t rcStBasis, rcStGedrueckt;
static lv_style_t rcStFarbe[7];
static bool rcStylesBereit = false;
enum { RC_ZIFFER, RC_OP, RC_CLEAR, RC_BACK, RC_PROZENT, RC_VZ, RC_GLEICH };
static const uint32_t RC_FARBEN[7] = { 0x34495e, 0x3498db, 0xe74c3c, 0xe67e22, 0x9b59b6, 0x16a085, 0x2ecc71 };

static String rcFormat(double v) {
  if (isnan(v) || isinf(v)) return "Fehler";
  char b[32];
  snprintf(b, sizeof(b), "%.10g", v);
  return String(b);
}
static String rcAnzeige(String s) { s.replace('.', ','); return s; }
static const char* rcOpText(char op) {
  switch (op) { case '+': return "+"; case '-': return "-"; case '*': return "x"; case '/': return "/"; }
  return "";
}

static void rcAktualisiere() {
  if (!rcWert || !rcFormel) return;
  lv_label_set_text(rcWert, rcFehler ? "Fehler" : rcAnzeige(rcEingabe).c_str());
  lv_label_set_text(rcFormel, rcFormelText.c_str());
}

static void rcAlles() {
  rcEingabe = "0"; rcFormelText = ""; rcAkku = 0; rcOp = 0; rcNeu = true; rcFehler = false;
}

static void rcFehlerSetzen(const String& formel) {
  rcFehler = true; rcOp = 0; rcNeu = true; rcFormelText = formel;
  rcAktualisiere();
}

static bool rcRechne(double a, char op, double b, double& out) {
  switch (op) {
    case '+': out = a + b; return true;
    case '-': out = a - b; return true;
    case '*': out = a * b; return true;
    case '/': if (b == 0) return false; out = a / b; return true;
  }
  out = b; return true;
}

static void rcZiffer(char c) {
  if (rcFehler) rcAlles();
  if (rcNeu) { rcEingabe = ""; rcNeu = false; }
  if (rcEingabe == "0") rcEingabe = "";
  if (rcEingabe.length() >= 12) return;
  rcEingabe += c;
  rcAktualisiere();
}

static void rcKomma() {
  if (rcFehler) rcAlles();
  if (rcNeu) { rcEingabe = "0"; rcNeu = false; }
  if (rcEingabe.indexOf('.') < 0 && rcEingabe.length() < 11) rcEingabe += ".";
  rcAktualisiere();
}

static void rcOperator(char op) {
  if (rcFehler) return;
  double wert = rcEingabe.toDouble();
  if (!rcOp) {
    rcAkku = wert;                                  // erster Operator
  } else if (!rcNeu) {                              // Kettenrechnung: 2 + 3 + ... rechnet zuerst 2 + 3
    double erg;
    String formel = rcAnzeige(rcFormat(rcAkku)) + " " + rcOpText(rcOp) + " " + rcAnzeige(rcEingabe);
    if (!rcRechne(rcAkku, rcOp, wert, erg)) { rcFehlerSetzen(formel + " ="); return; }
    rcAkku = erg;
  }                                                 // (Operator direkt nach Operator: nur wechseln)
  rcOp = op;
  rcNeu = true;
  rcEingabe = rcFormat(rcAkku);
  rcFormelText = rcAnzeige(rcEingabe) + " " + rcOpText(op);
  rcAktualisiere();
}

static void rcGleich() {
  if (rcFehler || !rcOp) return;
  double b = rcEingabe.toDouble(), erg;
  String formel = rcAnzeige(rcFormat(rcAkku)) + " " + rcOpText(rcOp) + " " + rcAnzeige(rcEingabe) + " =";
  if (!rcRechne(rcAkku, rcOp, b, erg)) { rcFehlerSetzen(formel); return; }
  rcAkku = erg;
  rcEingabe = rcFormat(erg);
  rcFormelText = formel;
  rcOp = 0;
  rcNeu = true;
  rcAktualisiere();
}

static void rcProzent() {
  if (rcFehler) return;
  double w = rcEingabe.toDouble();
  w = (rcOp == '+' || rcOp == '-') ? rcAkku * w / 100.0 : w / 100.0;   // 200 + 10 % = 200 + 20
  rcEingabe = rcFormat(w);
  rcNeu = false;
  rcAktualisiere();
}

static void rcVorzeichen() {
  if (rcFehler || rcEingabe == "0") return;
  if (rcEingabe.startsWith("-")) rcEingabe.remove(0, 1); else rcEingabe = "-" + rcEingabe;
  rcAktualisiere();
}

static void rcRueckschritt() {
  if (rcFehler) { rcAlles(); rcAktualisiere(); return; }
  if (rcNeu) return;
  rcEingabe.remove(rcEingabe.length() - 1);
  if (rcEingabe.length() == 0 || rcEingabe == "-") rcEingabe = "0";
  rcAktualisiere();
}

static void rcSchliessen() {
  nav_goto(SCREEN_ID_SCREENSETTING);                // zurück zu den Einstellungen; Rechner ist danach nicht mehr aktiv
  if (rcScreen) lv_obj_del_async(rcScreen);         // erst nach dem Callback löschen (die Taste ist Kind des Screens)
  rcScreen = rcWert = rcFormel = nullptr;
}

static void cbRechnerTaste(lv_event_t* e) {
  char c = (char)(intptr_t)lv_event_get_user_data(e);
  switch (c) {
    case 'C': rcAlles(); rcAktualisiere(); break;
    case 'B': rcRueckschritt(); break;
    case 'P': rcProzent(); break;
    case 'N': rcVorzeichen(); break;
    case '.': rcKomma(); break;
    case '=': rcGleich(); break;
    case '+': case '-': case '*': case '/': rcOperator(c); break;
    case 'X': rcSchliessen(); break;
    default:  if (c >= '0' && c <= '9') rcZiffer(c); break;
  }
}

static void rcStylesAnlegen() {
  if (rcStylesBereit) return;
  lv_style_init(&rcStBasis);
  lv_style_set_radius(&rcStBasis, 14);
  lv_style_set_border_width(&rcStBasis, 0);
  lv_style_set_shadow_width(&rcStBasis, 0);
  lv_style_set_outline_width(&rcStBasis, 0);
  lv_style_set_bg_opa(&rcStBasis, LV_OPA_COVER);
  lv_style_set_text_color(&rcStBasis, lv_color_white());
  lv_style_set_text_font(&rcStBasis, &font_montserrat_48);
  lv_style_init(&rcStGedrueckt);
  lv_style_set_bg_opa(&rcStGedrueckt, LV_OPA_60);
  for (int i = 0; i < 7; i++) {
    lv_style_init(&rcStFarbe[i]);
    lv_style_set_bg_color(&rcStFarbe[i], lv_color_hex(RC_FARBEN[i]));
  }
  rcStylesBereit = true;
}

static void erstelleRechnerScreen() {
  rcStylesAnlegen();
  rcScreen = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(rcScreen, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(rcScreen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(rcScreen, LV_OBJ_FLAG_SCROLLABLE);

  // Anzeige
  lv_obj_t* panel = lv_obj_create(rcScreen);
  lv_obj_set_pos(panel, 20, 14);
  lv_obj_set_size(panel, 984, 150);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x1b2631), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(panel, 14, 0);
  lv_obj_set_style_border_width(panel, 0, 0);
  lv_obj_set_style_pad_all(panel, 0, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  rcFormel = lv_label_create(panel);
  lv_label_set_long_mode(rcFormel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(rcFormel, 820);
  lv_obj_set_pos(rcFormel, 20, 8);
  lv_obj_set_style_text_font(rcFormel, &font_montserrat_32, 0);
  lv_obj_set_style_text_color(rcFormel, lv_color_hex(0x85c1e9), 0);

  rcWert = lv_label_create(panel);
  lv_obj_set_width(rcWert, 944);
  lv_obj_set_style_text_align(rcWert, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(rcWert, 20, 62);
  lv_obj_set_style_text_font(rcWert, &font_montserrat_62, 0);
  lv_obj_set_style_text_color(rcWert, lv_color_white(), 0);

  // Schließen oben rechts in der Anzeige
  lv_obj_t* ex = lv_btn_create(panel);
  lv_obj_set_pos(ex, 884, 8);
  lv_obj_set_size(ex, 90, 52);
  lv_obj_set_style_bg_color(ex, lv_color_hex(0xcc0000), 0);
  lv_obj_set_style_radius(ex, 10, 0);
  lv_obj_set_style_shadow_width(ex, 0, 0);
  lv_obj_t* exl = lv_label_create(ex);
  lv_label_set_text(exl, LV_SYMBOL_CLOSE);                       // Symbol: eingebaute Schrift
  lv_obj_set_style_text_font(exl, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(exl, lv_color_white(), 0);
  lv_obj_center(exl);
  lv_obj_add_event_cb(ex, cbRechnerTaste, LV_EVENT_CLICKED, (void*)(intptr_t)'X');

  // Tastenfeld 4 x 5
  struct Taste { char code; const char* text; int art; };
  static const Taste TASTEN[5][4] = {
    { {'C', "C", RC_CLEAR}, {'B', LV_SYMBOL_LEFT, RC_BACK}, {'P', "%", RC_PROZENT}, {'/', "/", RC_OP} },
    { {'7', "7", RC_ZIFFER}, {'8', "8", RC_ZIFFER}, {'9', "9", RC_ZIFFER}, {'*', "x", RC_OP} },
    { {'4', "4", RC_ZIFFER}, {'5', "5", RC_ZIFFER}, {'6', "6", RC_ZIFFER}, {'-', "-", RC_OP} },
    { {'1', "1", RC_ZIFFER}, {'2', "2", RC_ZIFFER}, {'3', "3", RC_ZIFFER}, {'+', "+", RC_OP} },
    { {'N', "+/-", RC_VZ}, {'0', "0", RC_ZIFFER}, {'.', ",", RC_ZIFFER}, {'=', "=", RC_GLEICH} },
  };
  for (int r = 0; r < 5; r++) {
    for (int k = 0; k < 4; k++) {
      const Taste& t = TASTEN[r][k];
      lv_obj_t* b = lv_btn_create(rcScreen);
      lv_obj_set_pos(b, 20 + k * 249, 180 + r * 84);
      lv_obj_set_size(b, 237, 72);
      lv_obj_add_style(b, &rcStBasis, 0);
      lv_obj_add_style(b, &rcStFarbe[t.art], 0);
      lv_obj_add_style(b, &rcStGedrueckt, LV_STATE_PRESSED);
      lv_obj_t* l = lv_label_create(b);
      lv_label_set_text(l, t.text);
      if (t.code == 'B') lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);   // Symbol: eingebaute Schrift
      lv_obj_center(l);
      lv_obj_add_event_cb(b, cbRechnerTaste, LV_EVENT_CLICKED, (void*)(intptr_t)t.code);
    }
  }
}

static size_t lvglFreiBytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

static void cbOpenRechner(lv_event_t*) {
  size_t vorher = lvglFreiBytes();
  if (vorher < 30000) {   // sonst droht ein LVGL-Out-of-Memory beim Anlegen
    logDiag("[Rechner] zu wenig Speicher: frei %u", (unsigned)vorher);
    return;
  }
  if (rcScreen) { lv_obj_del(rcScreen); rcScreen = rcWert = rcFormel = nullptr; }   // Rest eines nie geschlossenen Rechners
  rcAlles();
  erstelleRechnerScreen();
  rcAktualisiere();
  lv_scr_load(rcScreen);
  size_t nachher = lvglFreiBytes();
  logDiag("[Rechner] geoeffnet, LVGL-Speicher frei: %u (vorher %u), Bedarf %d Bytes", (unsigned)nachher, (unsigned)vorher, (int)((long)vorher - (long)nachher));
}

void uiRechnerInit() {
  if (objects.buttoncalc) lv_obj_add_event_cb(objects.buttoncalc, cbOpenRechner, LV_EVENT_CLICKED, nullptr);
}
