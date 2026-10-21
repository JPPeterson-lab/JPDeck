#include "settings_extras.h"
#include <Arduino.h>
#include "profiles.h"
#include "../ui/ui.h"
#include "../ui/colors.h"
#include "../ui/fonts/fonts.h"

static lv_obj_t *s_box, *s_line;
static lv_obj_t *s_title1, *s_state1, *s_hint1, *s_sw1;     // Automatischer Profilwechsel
static lv_obj_t *s_title2, *s_state2, *s_hint2, *s_sw2;     // Tasten am PC ausfuehren
static volatile uint32_t s_helperMs = 0;                    // Zeitpunkt der letzten Meldung des PC-Helfers (0 = noch nie)
#define HELPER_TIMEOUT_MS 10000

void settingsExtrasHelperSeen() { s_helperMs = millis() ? millis() : 1; }
bool settingsExtrasHelperAlive() { const uint32_t t = s_helperMs; return t && (uint32_t)(millis() - t) < HELPER_TIMEOUT_MS; }

static lv_color_t themeColor(int idx) { return lv_color_hex(theme_colors[active_theme_index][idx] & 0xFFFFFF); }

void settingsExtrasApplyTheme()
{
    if (!s_box) return;
    lv_obj_set_style_bg_color(s_box, themeColor(THEME_COLOR_BACKGR_CONT), 0);
    lv_obj_set_style_bg_color(s_line, themeColor(THEME_COLOR_TEXT), 0);
    for (lv_obj_t* l : { s_title1, s_state1, s_hint1, s_title2, s_state2, s_hint2 }) lv_obj_set_style_text_color(l, themeColor(THEME_COLOR_TEXT), 0);
    for (lv_obj_t* sw : { s_sw1, s_sw2 }) lv_obj_set_style_bg_color(sw, themeColor(THEME_COLOR_HIGHLITE_2), LV_PART_INDICATOR | LV_STATE_CHECKED);
}

static void cbSwitch1(lv_event_t*) { profilesSetAutoSwitch(lv_obj_has_state(s_sw1, LV_STATE_CHECKED)); settingsExtrasTick(); }
static void cbSwitch2(lv_event_t*) { profilesSetHostActions(lv_obj_has_state(s_sw2, LV_STATE_CHECKED)); settingsExtrasTick(); }

static lv_obj_t* mkLabel(const lv_font_t* f, int x, int y, int w, const char* text)
{
    lv_obj_t* l = lv_label_create(s_box);
    lv_obj_set_pos(l, x, y);
    if (w) { lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP); lv_obj_set_width(l, w); }
    lv_obj_set_style_text_font(l, f, 0);
    lv_label_set_text(l, text);
    return l;
}

static lv_obj_t* mkSwitch(int y, bool on, lv_event_cb_t cb)
{
    lv_obj_t* sw = lv_switch_create(s_box);
    lv_obj_set_size(sw, 90, 44);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -30, y);
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, cb, LV_EVENT_VALUE_CHANGED, nullptr);
    return sw;
}

void settingsExtrasInit()
{
    s_box = lv_obj_create(objects.screensetting);
    lv_obj_set_pos(s_box, 33, 288);                       // fluchtet mit den beiden Containern darueber (PicoPixel-Export)
    lv_obj_set_size(s_box, 958, 250);
    lv_obj_clear_flag(s_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_box, 5, 0);
    lv_obj_set_style_pad_all(s_box, 0, 0);
    lv_obj_set_style_border_width(s_box, 1, 0);          // wie die Container darueber: schwarzer Rand 1 px
    lv_obj_set_style_border_color(s_box, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_box, LV_OPA_COVER, 0);

    s_title1 = mkLabel(&font_montserrat_22, 20, 12, 0, "Automatischer Profilwechsel");
    s_state1 = mkLabel(&font_montserrat_18, 20, 46, 0, "");
    s_hint1  = mkLabel(&font_montserrat_18, 20, 72, 840,
                       "Wechselt das Profil nach dem Programm im Vordergrund (Feld \"Programm\" im Profil, sonst \"Standard\"). Braucht den DeckPad-Helfer auf dem PC.");
    s_sw1 = mkSwitch(8, profilesAutoSwitch(), cbSwitch1);

    s_line = lv_obj_create(s_box);                        // Trennlinie
    lv_obj_set_pos(s_line, 20, 124);
    lv_obj_set_size(s_line, 918, 1);
    lv_obj_set_style_border_width(s_line, 0, 0);
    lv_obj_set_style_radius(s_line, 0, 0);
    lv_obj_set_style_bg_opa(s_line, LV_OPA_40, 0);
    lv_obj_clear_flag(s_line, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    s_title2 = mkLabel(&font_montserrat_22, 20, 138, 0, "Tasten am PC ausf\xc3\xbchren");
    s_state2 = mkLabel(&font_montserrat_18, 20, 172, 0, "");
    s_hint2  = mkLabel(&font_montserrat_18, 20, 198, 840,
                       "Der DeckPad-Helfer f\xc3\xbchrt Tasten, Medientasten und Programmstarts am PC aus, statt dass das Board als USB-Tastatur auftritt. F\xc3\xbcr Boards ohne USB-Tastatur.");
    s_sw2 = mkSwitch(134, profilesHostActions(), cbSwitch2);

    settingsExtrasApplyTheme();
    settingsExtrasTick();
}

static void setIfChanged(lv_obj_t* l, const char* t) { if (strcmp(lv_label_get_text(l), t) != 0) lv_label_set_text(l, t); }

void settingsExtrasTick()
{
    if (!s_box) return;
    const bool up = settingsExtrasHelperAlive();
    setIfChanged(s_state1, !profilesAutoSwitch() ? "Aus" : up ? "An - Helfer auf dem PC aktiv" : "An - kein Helfer auf dem PC erkannt");
    setIfChanged(s_state2, !profilesHostActions() ? "Aus - das Board sendet die Tasten selbst per USB" : up ? "An - Helfer auf dem PC aktiv" : "An - kein Helfer erkannt, Tasten tun nichts");
}
