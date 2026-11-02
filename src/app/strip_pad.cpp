// Streifen-Seiten (siehe strip_pad.h).
#include "strip_pad.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include "profiles.h"
#include "navigation.h"
#include "diag_log.h"
#include "../ui/screens.h"
#include "../ui/colors.h"
#include "../ui/fonts/fonts.h"
#include "../display/lvgl_port.h"
#include "../usb/usb_device.h"

enum Mode { M_SCROLL, M_HSCROLL, M_TIMELINE, M_ZOOM, M_COUNT };
struct ModeInfo { const char* id; const char* name; const char* hint; bool horizontal; };
static const ModeInfo MODES[M_COUNT] = {
  { "scroll",   "Scrollen",           "Senkrecht ziehen scrollt.\nFinger nach unten = Inhalt nach unten.",                    false },
  { "hscroll",  "Waagerecht scrollen","Waagerecht ziehen scrollt zur Seite.",                                                  true  },
  { "timeline", "Zeitleiste",         "Waagerecht ziehen springt Bild fuer Bild (Pfeiltasten).\n2 Finger = grosse Schritte (Shift).", true  },
  { "zoom",     "Zoom",               "Senkrecht ziehen zoomt (Strg + Mausrad).\nFinger nach oben = hineinzoomen.",           false },
};

static int modeIndex(const char* id)
{
  if (id && *id) for (int i = 0; i < M_COUNT; i++) if (strcasecmp(id, MODES[i].id) == 0) return i;
  return -1;
}
bool stripPadModeValid(const char* id) { return !id || !*id || modeIndex(id) >= 0; }

static const int PAD_X = 20, PAD_Y = 84, PAD_W = 730, PAD_H = 496;
static const int N_TICKS = 14;
static const float TICK_GAP = 40.0f;
static const float NOTCH_PX = 22.0f;           // Fingerweg je Mausrad-Raste bei Empfindlichkeit 1,0
static const float STEP_PX  = 20.0f;           // Fingerweg je Pfeiltaste bei Empfindlichkeit 1,0

static lv_obj_t *scr, *padObj, *lblTitle, *lblStatus, *lblHint, *lblSens, *btnInv, *ticks[N_TICKS], *finger[2];
static lv_timer_t* tickTimer = nullptr;
static lv_style_t stBtn, stBtnPressed, stFinger, stTick;
static bool stylesReady = false;
static int  mode = M_SCROLL;

static struct {
  int   n = 0, settle = 0;
  float prev = 0, acc = 0, ticksOff = 0;
  bool  ctrlHeld = false;
  uint32_t lastMs = 0, lastDraw = 0;
} G;

enum { A_CLOSE, A_SENS_DOWN, A_SENS_UP, A_INV };

static lv_color_t themeColor(int idx) { return lv_color_hex(theme_colors[active_theme_index][idx] & 0xFFFFFF); }

static void release()
{
  if (G.ctrlHeld) { usbKeyboardMods(0); G.ctrlHeld = false; }
}

static void refreshLabels()
{
  if (!scr) return;
  const DeckNavSettings ns = profilesNav();
  char b[48];
  snprintf(b, sizeof(b), "Streifen  -  %s", MODES[mode].name);
  lv_label_set_text(lblTitle, b);
  lv_label_set_text(lblHint, MODES[mode].hint);
  snprintf(b, sizeof(b), "Empfindlichkeit  %d.%dx", ns.stripSens / 10, ns.stripSens % 10);
  lv_label_set_text(lblSens, b);
  lv_label_set_text(lv_obj_get_child(btnInv, 0), ns.stripInv ? "Richtung: umgekehrt" : "Richtung: normal");
}

static void closeScreen()
{
  if (tickTimer) { lv_timer_del(tickTimer); tickTimer = nullptr; }
  release();
  lvglPortSetTouchPeriod(0);
  nav_goto(SCREEN_ID_SCREEN_1);
  if (scr) lv_obj_del_async(scr);
  scr = padObj = lblTitle = lblStatus = lblHint = lblSens = btnInv = nullptr;
  for (auto& t : ticks) t = nullptr;
  for (auto& f : finger) f = nullptr;
}

static void cbBtn(lv_event_t* e)
{
  const int id = (int)(intptr_t)lv_event_get_user_data(e);
  DeckNavSettings ns = profilesNav();
  switch (id) {
    case A_CLOSE: closeScreen(); return;
    case A_SENS_DOWN: if (ns.stripSens > 5)  ns.stripSens -= (ns.stripSens > 20 ? 5 : 1); break;
    case A_SENS_UP:   if (ns.stripSens < 40) ns.stripSens += (ns.stripSens >= 20 ? 5 : 1); break;
    case A_INV: ns.stripInv = !ns.stripInv; break;
  }
  profilesSetNav(ns);
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

static void layoutTicks()
{
  const bool horiz = MODES[mode].horizontal;
  for (int i = 0; i < N_TICKS; i++) {
    if (!ticks[i]) continue;
    float pos = fmodf(G.ticksOff + i * TICK_GAP, N_TICKS * TICK_GAP);
    if (pos < 0) pos += N_TICKS * TICK_GAP;
    if (horiz) { lv_obj_set_size(ticks[i], 4, PAD_H - 80); lv_obj_set_pos(ticks[i], (int)pos - 20, 40); }
    else       { lv_obj_set_size(ticks[i], PAD_W - 80, 4); lv_obj_set_pos(ticks[i], 40, (int)pos - 20); }
    const int span = horiz ? PAD_W : PAD_H;
    if ((int)pos - 20 < 8 || (int)pos - 20 > span - 12) lv_obj_add_flag(ticks[i], LV_OBJ_FLAG_HIDDEN); else lv_obj_clear_flag(ticks[i], LV_OBJ_FLAG_HIDDEN);
  }
}

static void tickCb(lv_timer_t*)
{
  if (!scr) return;
  LvglTouchSnapshot t;
  lvglPortTouch(t);
  float fx[2], fy[2]; int n = 0;
  for (int i = 0; i < t.n && n < 2; i++)
    if (t.x[i] >= PAD_X && t.x[i] < PAD_X + PAD_W && t.y[i] >= PAD_Y && t.y[i] < PAD_Y + PAD_H) { fx[n] = t.x[i]; fy[n] = t.y[i]; n++; }
  for (int i = 0; i < 2; i++) {
    if (!finger[i]) continue;
    if (i < n) { lv_obj_clear_flag(finger[i], LV_OBJ_FLAG_HIDDEN); lv_obj_set_pos(finger[i], (int)fx[i] - PAD_X - 36, (int)fy[i] - PAD_Y - 36); }
    else lv_obj_add_flag(finger[i], LV_OBJ_FLAG_HIDDEN);
  }

  const bool horiz = MODES[mode].horizontal;
  const float pos = n ? (horiz ? (fx[0] + (n > 1 ? fx[1] : fx[0])) / 2 : (fy[0] + (n > 1 ? fy[1] : fy[0])) / 2) : 0;
  const DeckNavSettings ns = profilesNav();
  const float gain = ns.stripSens / 10.0f;

  if (n != G.n) {                                      // Fingerzahl gewechselt: neuer Nullpunkt
    if (n == 0) release();
    G.n = n; G.settle = 2; G.prev = pos; G.acc = 0;
    lv_label_set_text(lblStatus, n ? "..." : "bereit");
    return;
  }
  if (n == 0) return;
  if (G.settle > 0) { G.settle--; G.prev = pos; return; }

  const float d = pos - G.prev;
  G.prev = pos;
  if (d == 0) return;
  G.ticksOff += d;
  G.acc += d * gain;
  const float sgn = ns.stripInv ? -1.0f : 1.0f;

  switch (mode) {
    case M_SCROLL: {                                   // Finger nach unten = Rad positiv (Inhalt rutscht nach unten, wie auf dem Handy)
      int k = (int)(G.acc / NOTCH_PX); G.acc -= k * NOTCH_PX;
      if (k) usbMouseSend(0, 0, 0, (int8_t)constrain((int)(k * sgn), -127, 127), 0);
      break;
    }
    case M_HSCROLL: {                                  // Finger nach links = waagerechtes Rad positiv
      int k = (int)(G.acc / NOTCH_PX); G.acc -= k * NOTCH_PX;
      if (k) usbMouseSend(0, 0, 0, 0, (int8_t)constrain((int)(-k * sgn), -127, 127));
      break;
    }
    case M_ZOOM: {                                     // Finger nach oben = hineinzoomen (Rad positiv), Strg gehalten
      int k = (int)(G.acc / NOTCH_PX); G.acc -= k * NOTCH_PX;
      if (k) {
        if (!G.ctrlHeld) { usbKeyboardMods(0x01); G.ctrlHeld = true; }
        usbMouseSend(0, 0, 0, (int8_t)constrain((int)(-k * sgn), -127, 127), 0);
      }
      break;
    }
    case M_TIMELINE: {                                 // Rechts ziehen = Pfeil rechts; zwei Finger = Shift (groessere Schritte)
      int k = (int)(G.acc / STEP_PX); G.acc -= k * STEP_PX;
      k = (int)(k * sgn);
      const uint8_t mods = n >= 2 ? 0x02 : 0;
      for (int i = 0; i < abs(k) && i < 6; i++) usbKeyTapAsync(k > 0 ? 0x4F : 0x50, mods);
      break;
    }
  }
  layoutTicks();
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
    lv_style_set_bg_color(&stFinger, lv_color_hex(0x87ceeb));
    lv_style_set_bg_opa(&stFinger, LV_OPA_50);
    lv_style_set_shadow_width(&stFinger, 0);
    lv_style_init(&stTick);
    lv_style_set_radius(&stTick, 2);
    lv_style_set_border_width(&stTick, 0);
    lv_style_set_bg_color(&stTick, lv_color_hex(0x87ceeb));
    lv_style_set_bg_opa(&stTick, LV_OPA_30);
    lv_style_set_shadow_width(&stTick, 0);
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
  lv_obj_set_width(lblStatus, 240);
  lv_obj_set_pos(lblStatus, 764, 28);
  lv_obj_set_style_text_align(lblStatus, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_font(lblStatus, &font_montserrat_22, 0);
  lv_obj_set_style_text_color(lblStatus, themeColor(THEME_COLOR_TEXT), 0);
  lv_label_set_text(lblStatus, "bereit");

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
  for (int i = 0; i < N_TICKS; i++) {
    ticks[i] = lv_obj_create(padObj);
    lv_obj_add_style(ticks[i], &stTick, 0);
    lv_obj_clear_flag(ticks[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  }
  lblHint = lv_label_create(padObj);
  lv_obj_set_width(lblHint, PAD_W - 140);
  lv_obj_set_style_text_align(lblHint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(lblHint, &font_montserrat_22, 0);
  lv_obj_set_style_text_color(lblHint, themeColor(THEME_COLOR_TEXT), 0);
  lv_obj_set_style_text_opa(lblHint, LV_OPA_60, 0);
  lv_obj_center(lblHint);
  for (int i = 0; i < 2; i++) {
    finger[i] = lv_obj_create(padObj);
    lv_obj_set_size(finger[i], 72, 72);
    lv_obj_add_style(finger[i], &stFinger, 0);
    lv_obj_clear_flag(finger[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(finger[i], LV_OBJ_FLAG_HIDDEN);
  }

  const int X = 770, W = 234;
  lblSens = lv_label_create(scr);
  lv_obj_set_pos(lblSens, X, 92);
  lv_obj_set_style_text_font(lblSens, &font_montserrat_22, 0);
  lv_obj_set_style_text_color(lblSens, themeColor(THEME_COLOR_TEXT), 0);
  makeBtn(scr, X, 128, 112, 52, "-", 0x34495e, A_SENS_DOWN, &font_montserrat_32);
  makeBtn(scr, X + 122, 128, 112, 52, "+", 0x34495e, A_SENS_UP, &font_montserrat_32);
  btnInv = makeBtn(scr, X, 196, W, 56, "", 0x8e44ad, A_INV, &font_montserrat_22);
  layoutTicks();
  refreshLabels();
}

void stripPadOpen(const char* id)
{
  const int idx = modeIndex(id);
  mode = idx >= 0 ? idx : M_SCROLL;
  if (scr) { if (tickTimer) { lv_timer_del(tickTimer); tickTimer = nullptr; } lv_obj_del(scr); scr = nullptr; }
  release();
  G = decltype(G)();
  buildScreen();
  lv_scr_load(scr);
  lvglPortSetTouchPeriod(10);
  tickTimer = lv_timer_create(tickCb, 10, nullptr);
  logDiag("[Streifen] geoeffnet: %s", MODES[mode].name);
}
