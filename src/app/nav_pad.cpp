// 3D-Navigation per Touch (siehe nav_pad.h).
//
// Ablauf: Solange die Seite offen ist, liest ein LVGL-Timer alle 10 ms die Finger auf der Flaeche aus und erzeugt daraus
//   1 Finger  -> Ansicht drehen (Orbit)    : Maustaste (+ Shift) gedrueckt halten und bewegen
//   2 Finger  -> Ansicht schwenken (Pan)   : andere Maustaste gedrueckt halten, Mittelpunkt der Finger bewegen
//               und zoomen                 : Auseinanderziehen/Zusammenziehen als Mausrad-Rasten
// Der erste Kontakt legt den Nullpunkt fest, egal wo auf der Flaeche: so geht es ohne Hinsehen.
// Zwei Betriebsarten: "Touchpad" ueberträgt die Bewegung 1:1; "Joystick" macht die Auslenkung zur Geschwindigkeit
// (wie der Kopf einer Space Mouse), man muss dann nicht absetzen und neu ansetzen.
#include "nav_pad.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include "profiles.h"
#include "navigation.h"
#include "deck_actions.h"
#include "diag_log.h"
#include "../ui/screens.h"
#include "../ui/colors.h"
#include "../ui/fonts/fonts.h"
#include "../display/lvgl_port.h"
#include "../usb/usb_device.h"

// ── Voreinstellungen der Programme ────────────────────────────────────────────
enum { B_L = 1, B_R = 2, B_M = 4 };                    // Maustasten (HID-Bits)
enum { M_CTRL = 1, M_SHIFT = 2, M_ALT = 4 };           // Modifier (HID-Bits)
struct Bind { uint8_t btn, mod; };
struct View { const char* label; const char* keys; };  // Taste fuer eine feste Ansicht (Tastenkuerzel wie in den Kacheln)
struct Roll { uint8_t left, right, mod; };            // Rollen per Tastendruck (HID-Code links/rechts + Modifier); left == 0: kein Tastenpaar
struct Preset {
  const char* id; const char* name; Bind orbit, pan; const char* hint; View views[4]; Roll roll;
};
static const Preset PRESETS[] = {
  { "fusion",  "Fusion 360",   { B_M, M_SHIFT }, { B_M, 0 }, nullptr,
    { { "Einpassen", "f6" }, { "Home", "home" }, { nullptr, nullptr }, { nullptr, nullptr } }, { 0, 0, 0 } },
  { "shapr3d", "Shapr3D",      { B_R, 0 },       { B_M, 0 }, nullptr,
    { { nullptr, nullptr }, { nullptr, nullptr }, { nullptr, nullptr }, { nullptr, nullptr } }, { 0, 0, 0 } },
  { "bambu",   "Bambu Studio", { B_L, 0 },       { B_R, 0 }, "Mauszeiger auf eine freie Stelle legen: Drehen nutzt die linke Taste und verschiebt sonst ein Objekt.",
    { { "Alles", "z" }, { "Iso", "0" }, { "Oben", "1" }, { "Vorne", "3" } }, { 0, 0, 0 } },
  { "blender", "Blender",      { B_M, 0 },       { B_M, M_SHIFT }, nullptr,
    { { "Alles", "home" }, { nullptr, nullptr }, { nullptr, nullptr }, { nullptr, nullptr } },
    { 0x5C, 0x5E, M_CTRL | M_SHIFT } },                // Blender: Ansicht rollen = Strg+Shift+Ziffernblock 4 / 6
};
static const int N_PRESETS = sizeof(PRESETS) / sizeof(PRESETS[0]);

static int presetIndex(const char* id)
{
  if (id && *id) for (int i = 0; i < N_PRESETS; i++) if (strcasecmp(id, PRESETS[i].id) == 0) return i;
  return -1;
}
bool navPadPresetValid(const char* id) { return !id || !*id || presetIndex(id) >= 0; }

// ── Gestenerkennung ───────────────────────────────────────────────────────────
// Rechenfenster der Fingerflaeche (Bildschirmkoordinaten)
static const int PAD_X = 20, PAD_Y = 84, PAD_W = 730, PAD_H = 496;
static const float MOVE_START_PX = 8.0f;     // erst ab dieser Bewegung wird die Maustaste gedrueckt (ein Tipp bleibt ein Tipp)
static const float ZOOM_STEP_PX  = 26.0f;    // Fingerabstand, der eine Mausrad-Raste ergibt (bei Empfindlichkeit 1,0)
static const float JOY_DEAD_PX   = 10.0f;    // Joystick: Totzone um den Nullpunkt
static const float JOY_ZOOM_DEAD = 22.0f;

struct Engine {
  int      n = 0, settle = 0;
  uint32_t lastMs = 0;
  float    orgX = 0, orgY = 0, orgDist = 0;    // Nullpunkt (Mittelpunkt) und Fingerabstand beim Aufsetzen
  float    baseX = 0, baseY = 0, baseDist = 0; // letzter Stand (Touchpad-Modus: Bewegung = Unterschied dazu)
  float    remX = 0, remY = 0, zoomAcc = 0;    // noch nicht gesendete Bruchteile
  bool     pressed = false;
  uint8_t  buttons = 0, mods = 0;
  float    rollPrev = 0, rollAcc = 0, phi = 0;  // 3 Finger: letzter Winkel, aufgelaufener Winkel bis zum naechsten Tastendruck, Winkel auf dem Kreis
};
static Engine E;

static const Preset& curPreset() { int i = profilesNav().preset; return PRESETS[i < N_PRESETS ? i : 0]; }

static void sendMove(int dx, int dy, int wheel)
{
  // HID-Werte sind 8 Bit: grosse Spruenge in Stuecke teilen
  for (int guard = 0; guard < 10 && (dx || dy || wheel); guard++) {
    const int sx = constrain(dx, -127, 127), sy = constrain(dy, -127, 127), sw = constrain(wheel, -127, 127);
    usbMouseSend(E.buttons, (int8_t)sx, (int8_t)sy, (int8_t)sw);
    dx -= sx; dy -= sy; wheel -= sw;
  }
}

static void pressGesture(const Bind& b)
{
  if (E.pressed) return;
  if (b.mod) usbKeyboardMods(b.mod);
  E.mods = b.mod; E.buttons = b.btn; E.pressed = true;
  usbMouseSend(b.btn, 0, 0, 0);
}

static void releaseGesture()
{
  if (E.pressed) {
    E.buttons = 0; usbMouseSend(0, 0, 0, 0);
    if (E.mods) usbKeyboardMods(0);
  }
  E.pressed = false; E.buttons = 0; E.mods = 0;
}

struct Pt { float x, y; };
static int collect(const LvglTouchSnapshot& t, Pt* out)   // nur Finger auf der Flaeche
{
  int k = 0;
  for (int i = 0; i < t.n && k < 3; i++)
    if (t.x[i] >= PAD_X && t.x[i] < PAD_X + PAD_W && t.y[i] >= PAD_Y && t.y[i] < PAD_Y + PAD_H) out[k++] = { (float)t.x[i], (float)t.y[i] };
  return k;
}

static float fingerAngle(const Pt* p)               // Winkel des ersten Fingers um den Mittelpunkt aller drei
{
  const float cx = (p[0].x + p[1].x + p[2].x) / 3, cy = (p[0].y + p[1].y + p[2].y) / 3;
  return atan2f(p[0].y - cy, p[0].x - cx);
}

static void capture(const Pt* p, int n)
{
  if (n == 3) { E.rollPrev = fingerAngle(p); E.rollAcc = 0; E.phi = 0; }
  if (n == 1) { E.orgX = E.baseX = p[0].x; E.orgY = E.baseY = p[0].y; E.orgDist = E.baseDist = 0; }
  else if (n == 2) {
    E.orgX = E.baseX = (p[0].x + p[1].x) / 2; E.orgY = E.baseY = (p[0].y + p[1].y) / 2;
    E.orgDist = E.baseDist = hypotf(p[1].x - p[0].x, p[1].y - p[0].y);
  }
  E.remX = E.remY = E.zoomAcc = 0;
}

static float joyVel(float d)              // Auslenkung in Pixel -> Zaehlschritte je 10 ms, leicht progressiv
{
  float a = fabsf(d) - JOY_DEAD_PX;
  if (a <= 0) return 0;
  float v = a * 0.06f * (1.0f + a / 150.0f);
  return d < 0 ? -v : v;
}

// Wird alle 10 ms aufgerufen. Gibt true zurueck, wenn eine Geste gerade etwas bewirkt (fuer die Statusanzeige).
static const char* gestureTick(const Pt* p, int n, float dt)
{
  const DeckNavSettings ns = profilesNav();
  const Preset& P = curPreset();
  const float gain = ns.sens / 10.0f;
  const bool joy = ns.mode == 1;
  const float k = dt * 100.0f;               // Joystick-Schritte auf die tatsaechliche Zeit umrechnen

  if (n == 1) {
    if (!E.pressed) {
      if (hypotf(p[0].x - E.orgX, p[0].y - E.orgY) < MOVE_START_PX) return "";
      pressGesture(P.orbit);
    }
    float mx, my;
    if (!joy) { mx = (p[0].x - E.baseX) * gain; my = (p[0].y - E.baseY) * gain; E.baseX = p[0].x; E.baseY = p[0].y; }
    else      { mx = joyVel(p[0].x - E.orgX) * gain * k; my = joyVel(p[0].y - E.orgY) * gain * k; }
    E.remX += mx; E.remY += my;
    int ix = (int)E.remX, iy = (int)E.remY;
    E.remX -= ix; E.remY -= iy;
    sendMove(ix, iy, 0);
    return "Drehen";
  }

  if (n == 2) {
    const float cx = (p[0].x + p[1].x) / 2, cy = (p[0].y + p[1].y) / 2;
    const float dist = hypotf(p[1].x - p[0].x, p[1].y - p[0].y);
    if (!E.pressed && hypotf(cx - E.orgX, cy - E.orgY) >= MOVE_START_PX) pressGesture(P.pan);
    float mx = 0, my = 0, zoom = 0;
    if (!joy) {
      mx = (cx - E.baseX) * gain; my = (cy - E.baseY) * gain;
      zoom = (dist - E.baseDist) * gain / ZOOM_STEP_PX;
      E.baseX = cx; E.baseY = cy; E.baseDist = dist;
    } else {
      mx = joyVel(cx - E.orgX) * gain * k; my = joyVel(cy - E.orgY) * gain * k;
      float dd = dist - E.orgDist, a = fabsf(dd) - JOY_ZOOM_DEAD;
      if (a > 0) zoom = (dd < 0 ? -1 : 1) * (a / 14.0f) * gain * dt;     // Raster pro Sekunde = Auslenkung / 14
    }
    if (E.pressed) { E.remX += mx; E.remY += my; } else { /* Schwenken erst nach der Schwelle */ }
    int ix = (int)E.remX, iy = (int)E.remY;
    E.remX -= ix; E.remY -= iy;
    E.zoomAcc += zoom;
    int notches = (int)E.zoomAcc;
    E.zoomAcc -= notches;
    if (ns.invZoom) notches = -notches;
    sendMove(ix, iy, notches);
    return "Schwenken + Zoom";
  }
  if (n == 3) {                                       // Rollen: Drehung der drei Finger um ihren Mittelpunkt
    const float a = fingerAngle(p);
    float da = a - E.rollPrev;
    if (da > (float)M_PI) da -= 2 * (float)M_PI; else if (da < -(float)M_PI) da += 2 * (float)M_PI;
    E.rollPrev = a;
    if (P.roll.left) {                                // Programm mit Tastenpaar (Blender): alle 12 Grad ein Tastendruck
      E.rollAcc += da * gain;
      const float step = 0.21f;
      while (E.rollAcc >= step)  { usbKeyTapAsync(P.roll.right, P.roll.mod); E.rollAcc -= step; }
      while (E.rollAcc <= -step) { usbKeyTapAsync(P.roll.left,  P.roll.mod); E.rollAcc += step; }
    } else {                                          // ohne Rollen im Programm: Mausbahn im Kreis (wirkt bei freiem Orbit/Trackball)
      if (!E.pressed) { if (fabsf(da) < 0.004f) return "Rollen"; pressGesture(P.orbit); }
      const float R = 180.0f * gain;
      E.phi += da;
      E.remX += -sinf(E.phi) * R * da;
      E.remY +=  cosf(E.phi) * R * da;
      int ix = (int)E.remX, iy = (int)E.remY;
      E.remX -= ix; E.remY -= iy;
      sendMove(ix, iy, 0);
    }
    return "Rollen";
  }
  return "";
}

// ── Oberflaeche ───────────────────────────────────────────────────────────────
static lv_obj_t *scr, *padObj, *lblTitle, *lblStatus, *lblHint, *lblSens;
static lv_obj_t *btnPreset, *btnMode, *btnZoom, *btnViews[4], *finger[5];
static lv_timer_t* tickTimer = nullptr;
static lv_style_t stBtn, stBtnPressed, stFinger;
static bool stylesReady = false;
enum { A_CLOSE, A_PRESET, A_MODE, A_ZOOM, A_SENS_DOWN, A_SENS_UP, A_VIEW0 } ;

static lv_color_t themeColor(int idx) { return lv_color_hex(theme_colors[active_theme_index][idx] & 0xFFFFFF); }

static void refreshLabels()
{
  if (!scr) return;
  const DeckNavSettings ns = profilesNav();
  const Preset& P = curPreset();
  char b[64];
  snprintf(b, sizeof(b), "3D-Navigation  -  %s", P.name);
  lv_label_set_text(lblTitle, b);
  lv_label_set_text(lv_obj_get_child(btnPreset, 0), P.name);
  lv_label_set_text(lv_obj_get_child(btnMode, 0), ns.mode ? "Modus: Joystick" : "Modus: Touchpad");
  lv_label_set_text(lv_obj_get_child(btnZoom, 0), ns.invZoom ? "Zoom: umgekehrt" : "Zoom: normal");
  snprintf(b, sizeof(b), "Empfindlichkeit  %d.%dx", ns.sens / 10, ns.sens % 10);
  lv_label_set_text(lblSens, b);
  for (int i = 0; i < 4; i++) {
    if (P.views[i].label) { lv_label_set_text(lv_obj_get_child(btnViews[i], 0), P.views[i].label); lv_obj_clear_flag(btnViews[i], LV_OBJ_FLAG_HIDDEN); }
    else lv_obj_add_flag(btnViews[i], LV_OBJ_FLAG_HIDDEN);
  }
  lv_label_set_text(lblHint, P.hint ? P.hint : (ns.mode ? "Joystick: Finger auslenken, Absetzen beendet.\n1 Finger drehen, 2 Finger schwenken, Abstand = Zoom, 3 Finger rollen." : "1 Finger drehen\n2 Finger schwenken\nAbstand der Finger = Zoom\n3 Finger drehen = Rollen"));
}

static void closeScreen()
{
  if (tickTimer) { lv_timer_del(tickTimer); tickTimer = nullptr; }
  releaseGesture();
  lvglPortSetTouchPeriod(0);
  nav_goto(SCREEN_ID_SCREEN_1);
  if (scr) lv_obj_del_async(scr);                      // erst nach dem Callback loeschen (die Taste ist Kind des Screens)
  scr = padObj = lblTitle = lblStatus = lblHint = lblSens = btnPreset = btnMode = btnZoom = nullptr;
  for (auto& b : btnViews) b = nullptr;
  for (auto& f : finger) f = nullptr;
}

static void cbBtn(lv_event_t* e)
{
  const int id = (int)(intptr_t)lv_event_get_user_data(e);
  DeckNavSettings ns = profilesNav();
  switch (id) {
    case A_CLOSE: closeScreen(); return;
    case A_PRESET: ns.preset = (ns.preset + 1) % N_PRESETS; break;
    case A_MODE:   ns.mode = !ns.mode; break;
    case A_ZOOM:   ns.invZoom = !ns.invZoom; break;
    case A_SENS_DOWN: if (ns.sens > 5)  ns.sens -= (ns.sens > 20 ? 5 : 1); break;
    case A_SENS_UP:   if (ns.sens < 40) ns.sens += (ns.sens >= 20 ? 5 : 1); break;
    default: {
      const View& v = curPreset().views[id - A_VIEW0];
      if (v.keys) deckActionRun(ACT_KEYS, v.keys);
      return;
    }
  }
  profilesSetNav(ns);
  releaseGesture();
  refreshLabels();
}

static lv_obj_t* makeBtn(lv_obj_t* parent, int x, int y, int w, int h, const char* text, uint32_t color, int id, const lv_font_t* font)
{
  lv_obj_t* b = lv_btn_create(parent);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_add_style(b, &stBtn, 0);
  lv_obj_add_style(b, &stBtnPressed, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(b, lv_color_hex(color), 0);
  lv_obj_t* l = lv_label_create(b);
  lv_label_set_text(l, text);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_center(l);
  lv_obj_add_event_cb(b, cbBtn, LV_EVENT_CLICKED, (void*)(intptr_t)id);
  return b;
}

static void moveFinger(int i, bool show, float x, float y)
{
  if (!finger[i]) return;
  if (!show) { lv_obj_add_flag(finger[i], LV_OBJ_FLAG_HIDDEN); return; }
  lv_obj_clear_flag(finger[i], LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_pos(finger[i], (int)x - PAD_X - 36, (int)y - PAD_Y - 36);
}

static uint8_t s_lastN = 0xFF;
static void tickCb(lv_timer_t*)
{
  if (!scr) return;
  LvglTouchSnapshot t;
  lvglPortTouch(t);
  Pt p[3];
  const int n = collect(t, p);
  const uint32_t now = millis();
  float dt = (now - E.lastMs) / 1000.0f;
  E.lastMs = now;
  if (dt < 0.002f) dt = 0.002f;
  if (dt > 0.05f) dt = 0.05f;

  for (int i = 0; i < 5; i++) moveFinger(i, i < n, i < n ? p[i].x : 0, i < n ? p[i].y : 0);

  const char* what = "";
  if (n != E.n) {                                      // Fingerzahl gewechselt: neue Geste, neuer Nullpunkt
    releaseGesture();
    E.n = n; E.settle = 2;
    capture(p, n);
  } else if (n >= 1 && n <= 3) {
    if (E.settle > 0) { E.settle--; capture(p, n); }   // die ersten Messungen nach dem Aufsetzen sind unruhig
    else what = gestureTick(p, n, dt);
  }

  if (n != s_lastN || what[0]) {
    s_lastN = (uint8_t)n;
    char b[48];
    if (!usbHostConnected()) lv_label_set_text(lblStatus, "PC nicht verbunden");
    else if (n == 0) lv_label_set_text(lblStatus, "bereit");
    else { snprintf(b, sizeof(b), "%d Finger  %s", n, what); lv_label_set_text(lblStatus, b); }
  }
}

static void buildScreen()
{
  if (!stylesReady) {
    lv_style_init(&stBtn);
    lv_style_set_radius(&stBtn, 14);
    lv_style_set_border_width(&stBtn, 0);
    lv_style_set_shadow_width(&stBtn, 0);
    lv_style_set_bg_opa(&stBtn, LV_OPA_COVER);
    lv_style_set_text_color(&stBtn, lv_color_white());
    lv_style_init(&stBtnPressed);
    lv_style_set_bg_opa(&stBtnPressed, LV_OPA_60);
    lv_style_init(&stFinger);
    lv_style_set_radius(&stFinger, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&stFinger, 0);
    lv_style_set_bg_color(&stFinger, lv_color_hex(0x87ceeb));       // Standardblau
    lv_style_set_bg_opa(&stFinger, LV_OPA_50);
    lv_style_set_shadow_width(&stFinger, 0);
    stylesReady = true;
  }
  scr = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(scr, themeColor(THEME_COLOR_SCREEN), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  makeBtn(scr, 20, 14, 150, 56, "Zur\xc3\xbc" "ck", 0x7f8c8d, A_CLOSE, &font_montserrat_32);
  lblTitle = lv_label_create(scr);
  lv_obj_set_pos(lblTitle, 190, 22);
  lv_obj_set_style_text_font(lblTitle, &font_montserrat_32, 0);
  lv_obj_set_style_text_color(lblTitle, themeColor(THEME_COLOR_TEXT), 0);
  lblStatus = lv_label_create(scr);
  lv_obj_set_width(lblStatus, 340);
  lv_obj_set_pos(lblStatus, 664, 28);
  lv_obj_set_style_text_align(lblStatus, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_font(lblStatus, &font_montserrat_22, 0);
  lv_obj_set_style_text_color(lblStatus, themeColor(THEME_COLOR_TEXT), 0);

  // Die leere Flaeche
  padObj = lv_obj_create(scr);
  lv_obj_set_pos(padObj, PAD_X, PAD_Y);
  lv_obj_set_size(padObj, PAD_W, PAD_H);
  lv_obj_clear_flag(padObj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(padObj, 18, 0);
  lv_obj_set_style_pad_all(padObj, 0, 0);
  lv_obj_set_style_border_width(padObj, 2, 0);
  lv_obj_set_style_border_color(padObj, lv_color_hex(0x87ceeb), 0);
  lv_obj_set_style_bg_color(padObj, themeColor(THEME_COLOR_BACKGR_CONT), 0);
  lv_obj_set_style_bg_opa(padObj, LV_OPA_COVER, 0);
  lblHint = lv_label_create(padObj);
  lv_obj_set_width(lblHint, PAD_W - 80);
  lv_obj_set_style_text_align(lblHint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(lblHint, &font_montserrat_22, 0);
  lv_obj_set_style_text_color(lblHint, themeColor(THEME_COLOR_TEXT), 0);
  lv_obj_set_style_text_opa(lblHint, LV_OPA_50, 0);
  lv_obj_center(lblHint);
  for (int i = 0; i < 5; i++) {
    finger[i] = lv_obj_create(padObj);
    lv_obj_set_size(finger[i], 72, 72);
    lv_obj_add_style(finger[i], &stFinger, 0);
    lv_obj_clear_flag(finger[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(finger[i], LV_OBJ_FLAG_HIDDEN);
  }

  // Rechte Spalte: Einstellungen und feste Ansichten
  const int X = 770, W = 234;
  btnPreset = makeBtn(scr, X, 84, W, 56, "", 0x3498db, A_PRESET, &font_montserrat_22);
  btnMode   = makeBtn(scr, X, 150, W, 56, "", 0x16a085, A_MODE, &font_montserrat_22);
  lblSens = lv_label_create(scr);
  lv_obj_set_pos(lblSens, X, 222);
  lv_obj_set_style_text_font(lblSens, &font_montserrat_22, 0);
  lv_obj_set_style_text_color(lblSens, themeColor(THEME_COLOR_TEXT), 0);
  makeBtn(scr, X, 256, 112, 52, "-", 0x34495e, A_SENS_DOWN, &font_montserrat_32);
  makeBtn(scr, X + 122, 256, 112, 52, "+", 0x34495e, A_SENS_UP, &font_montserrat_32);
  btnZoom = makeBtn(scr, X, 322, W, 56, "", 0x8e44ad, A_ZOOM, &font_montserrat_22);
  for (int i = 0; i < 4; i++)
    btnViews[i] = makeBtn(scr, X + (i % 2) * 122, 412 + (i / 2) * 70, 112, 60, "", 0xe67e22, A_VIEW0 + i, &font_montserrat_22);
  refreshLabels();
}

void navPadOpen(const char* presetId)
{
  const int idx = presetIndex(presetId);
  if (idx >= 0) { DeckNavSettings ns = profilesNav(); if (ns.preset != idx) { ns.preset = (uint8_t)idx; profilesSetNav(ns); } }
  if (scr) { if (tickTimer) { lv_timer_del(tickTimer); tickTimer = nullptr; } lv_obj_del(scr); scr = nullptr; }
  E = Engine();
  s_lastN = 0xFF;
  buildScreen();
  lv_scr_load(scr);
  lvglPortSetTouchPeriod(10);                          // Touch alle 10 ms statt alle 30 ms abfragen
  E.lastMs = millis();
  tickTimer = lv_timer_create(tickCb, 10, nullptr);
  logDiag("[3D] Navigation geoeffnet: %s", curPreset().name);
}
