#include "deck_ui.h"
#include "diag_log.h"
#include <Arduino.h>
#include <time.h>
#include "app_config.h"
#include "profiles.h"
#include "deck_actions.h"
#include "navigation.h"
#include "../ui/ui.h"
#include "../ui/colors.h"
#include "../ui/fonts/fonts.h"
#include "../usb/usb_device.h"
#include "../hardware/backlight.h"
#include "../display/lvgl_port.h"
#include "icons.h"
#include "settings_extras.h"

// ── Gemeinsame Styles (ein Satz fuer alle Kacheln statt Einzel-Styles: spart LVGL-Speicher) ──
static lv_style_t                 stTile, stTilePressed;
static lv_style_transition_dsc_t  trFade, trInstant;
static bool                       stylesReady = false;

static void initStyles()
{
    if (stylesReady) return;
    static const lv_style_prop_t props[] = { LV_STYLE_BG_COLOR, (lv_style_prop_t)0 };
    // Loslassen: die Farbe blendet in 350 ms zurueck ("Taste ändert kurz die Farbe").
    lv_style_transition_dsc_init(&trFade, props, lv_anim_path_ease_out, 350, 0, nullptr);
    // Druecken: sofort umschalten, keine Verzoegerung
    lv_style_transition_dsc_init(&trInstant, props, lv_anim_path_linear, 0, 0, nullptr);

    lv_style_init(&stTile);
    lv_style_set_radius(&stTile, 16);
    lv_style_set_border_width(&stTile, 0);
    lv_style_set_shadow_width(&stTile, 0);
    lv_style_set_outline_width(&stTile, 0);
    lv_style_set_bg_opa(&stTile, LV_OPA_COVER);
    lv_style_set_pad_all(&stTile, 6);
    lv_style_set_text_color(&stTile, lv_color_white());
    lv_style_set_text_font(&stTile, &font_montserrat_22);
    lv_style_set_text_align(&stTile, LV_TEXT_ALIGN_CENTER);
    lv_style_set_transition(&stTile, &trFade);

    lv_style_init(&stTilePressed);
    lv_style_set_transition(&stTilePressed, &trInstant);
    stylesReady = true;
}

// "PC verbunden" gruen hinterlegt, "Kein PC" rot (change_color_theme setzt die Textfarbe zurueck, daher neu anwenden)
static void styleStatus(bool up)
{
    lv_obj_t* l = objects.labelconnectionpc;
    lv_obj_set_style_text_color(l, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(l, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(l, lv_color_hex(up ? 0x1e8e4e : 0xc0392b), 0);
    lv_obj_set_style_radius(l, 6, 0);
}

// ── Kachelraster ──────────────────────────────────────────────────────────────
// Geladene Icons: Pixeldaten liegen im PSRAM und muessen so lange leben wie die Kacheln, die sie anzeigen.
struct IconSlot { lv_img_dsc_t dsc; uint8_t* data; };
static IconSlot s_icons[DECK_MAX_TILES];

static void freeIcons()
{
    for (auto& s : s_icons) { if (s.data) { heap_caps_free(s.data); s.data = nullptr; } }
}

static void cbTile(lv_event_t* e)
{
    const DeckProfile& p = profileActive();
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    if (i < 0 || i >= p.tileCount) return;
    const DeckTile& t = p.tiles[i];
    logDiag("[Taste] %d '%s' -> %s %s", i, t.label, deckActionTypeName(t.type), t.spec);
    deckActionRun(t.type, t.spec);          // sofort beim Berühren, nicht erst beim Loslassen: fühlt sich direkter an
}

static void buildGrid()
{
    lv_obj_t* g = objects.containergrid;
    if (!g) return;
    lv_obj_clean(g);
    freeIcons();                                            // erst nach dem Loeschen der Bilder, die darauf zeigten

    const DeckProfile& p = profileActive();
    const int gap = DECK_GRID_GAP;
    const int W = lv_obj_get_width(g), H = lv_obj_get_height(g);
    const int tw = (W - gap * (p.cols - 1)) / p.cols;
    const int th = (H - gap * (p.rows - 1)) / p.rows;
    const lv_color_t emptyColor = lv_color_hex(theme_colors[active_theme_index][THEME_COLOR_BACKGR_CONT] & 0xFFFFFF);

    int8_t bySlot[DECK_MAX_TILES];                          // Platz im Raster -> Index der Kachel (-1 = frei)
    for (auto& v : bySlot) v = -1;
    for (int k = 0; k < p.tileCount; k++) if (p.tiles[k].slot < DECK_MAX_TILES) bySlot[p.tiles[k].slot] = (int8_t)k;

    for (int r = 0; r < p.rows; r++) {
        for (int c = 0; c < p.cols; c++) {
            const int i = bySlot[r * p.cols + c];            // Index der Kachel an diesem Platz
            const int x = c * (tw + gap), y = r * (th + gap);
            if (i < 0) {                                    // freier Platz im Raster: dezente, nicht antippbare Fläche
                lv_obj_t* o = lv_obj_create(g);
                lv_obj_set_pos(o, x, y);
                lv_obj_set_size(o, tw, th);
                lv_obj_clear_flag(o, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_radius(o, 16, 0);
                lv_obj_set_style_border_width(o, 0, 0);
                lv_obj_set_style_bg_color(o, emptyColor, 0);
                lv_obj_set_style_bg_opa(o, LV_OPA_40, 0);
                continue;
            }
            const DeckTile& t = p.tiles[i];
            lv_obj_t* b = lv_btn_create(g);
            lv_obj_set_pos(b, x, y);
            lv_obj_set_size(b, tw, th);
            lv_obj_add_style(b, &stTile, 0);
            lv_obj_add_style(b, &stTilePressed, LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(b, lv_color_hex(t.color), 0);
            lv_obj_set_style_bg_color(b, lv_color_lighten(lv_color_hex(t.color), 110), LV_STATE_PRESSED);
            lv_obj_set_flex_flow(b, LV_FLEX_FLOW_COLUMN);           // Icon oben, Beschriftung darunter, beides mittig
            lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_row(b, 2, 0);
            if (t.icon[0] && iconLoad(t.icon, s_icons[i].dsc, &s_icons[i].data)) {
                lv_obj_t* im = lv_img_create(b);
                lv_img_set_src(im, &s_icons[i].dsc);
            }
            if (t.label[0] || !s_icons[i].data) {                   // ohne Icon immer eine Beschriftung (auch leer), mit Icon nur wenn Text da ist
                lv_obj_t* l = lv_label_create(b);
                lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
                lv_obj_set_width(l, tw - 12);
                lv_label_set_text(l, t.label);
            }
            lv_obj_add_event_cb(b, cbTile, LV_EVENT_PRESSED, (void*)(intptr_t)i);
        }
    }
}

// ── Profil-Schnelltasten: Standard und P1..P5 ──────────────────────────────────
// Sie liegen auf dem Deck-Bildschirm neben dem Kachelfeld und gelten damit fuer jedes Profil.
// "Standard" springt immer auf das Profil "standard"; P1..P5 auf das im Editor zugewiesene Profil.
static void switchProfile(int index);
static lv_obj_t* quickBtn(int n)
{
    lv_obj_t* const b[DECK_QUICK_COUNT] = { objects.buttonp1, objects.buttonp2, objects.buttonp3, objects.buttonp4, objects.buttonp5 };
    return n < 0 ? objects.buttonstandard : b[n];
}

static void updateQuickButtons()
{
    const char* activeId = profileInfo(profileActiveIndex()).id;
    for (int n = -1; n < DECK_QUICK_COUNT; n++) {
        lv_obj_t* b = quickBtn(n);
        if (!b) continue;
        const char* id = n < 0 ? "standard" : profilesQuickId(n);
        const bool assigned = id[0] && profileIndexOf(id) >= 0;
        if (assigned) lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE); else lv_obj_clear_flag(b, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_opa(b, assigned ? LV_OPA_COVER : LV_OPA_30, 0);        // nicht belegte Taste blass und ohne Funktion
        const bool isActive = assigned && strcmp(id, activeId) == 0;
        lv_obj_set_style_border_width(b, isActive ? 3 : 0, 0);                  // das gerade aktive Profil ist umrandet
        lv_obj_set_style_border_color(b, lv_color_hex(0x3498db), 0);
    }
}

static void cbQuick(lv_event_t* e)
{
    const int n = (int)(intptr_t)lv_event_get_user_data(e);
    const char* id = n < 0 ? "standard" : profilesQuickId(n);
    const int i = profileIndexOf(id);
    if (i >= 0) switchProfile(i);
}

void deckUiQuickChanged() { updateQuickButtons(); }

// ── Profil uebernehmen ────────────────────────────────────────────────────────
void deckUiApplyProfile()
{
    const DeckProfile& p = profileActive();
    change_color_theme(profilesTheme());                    // PicoPixel: alle Farben des Themes (gilt fuer alle Profile)
    styleStatus(usbHostConnected());                        // setzt die Textfarbe des Status-Labels zurueck, daher neu
    settingsExtrasApplyTheme();
    backlightSet(profilesBrightness());                     // Helligkeit und Design gelten fuer alle Profile
    lv_arc_set_value(objects.labellightchangevalue, profilesBrightness());
    lv_label_set_text(objects.labelprofil, p.name);
    lv_roller_set_selected(objects.rollerprofile, profileActiveIndex(), LV_ANIM_OFF);
    buildGrid();
    updateQuickButtons();
}

static void switchProfile(int index)
{
    if (index == profileActiveIndex()) return;
    if (profileActivate(index)) {
        const uint32_t t0 = millis();
        deckUiApplyProfile();
        logDiag("[UI] Profil gewechselt: %s (Aufbau %lu ms)", profileActive().name, (unsigned long)(millis() - t0));
    }
    else logDiag("[UI] Profil %d konnte nicht geladen werden", index);
}

void deckUiProfilesChanged()
{
    profilesRescan();                                       // Liste neu; aktives Profil bleibt per id erhalten
    String names;
    profileNamesJoined(names);
    lv_roller_set_options(objects.rollerprofile, names.c_str(), LV_ROLLER_MODE_NORMAL);
    profileActivate(profileActiveIndex());                  // aktives Profil neu aus der Datei laden (kann ueberschrieben worden sein)
    deckUiApplyProfile();
}

void deckUiRebuildGrid() { buildGrid(); }
void deckUiRedrawAll() { lv_obj_invalidate(lv_scr_act()); }

void deckUiAutoSwitch(const char* exe)
{
    if (!profilesAutoSwitch()) return;
    int i = profileIndexByExe(exe);
    if (i < 0) i = profileIndexOf("standard");                  // kein Profil fuer dieses Programm: Standard
    if (i < 0 || i == profileActiveIndex()) return;
    logDiag("[Auto] Vordergrund '%s' -> Profil '%s'", exe, profileInfo(i).name);
    switchProfile(i);
}

bool deckUiSelectProfile(const char* id)
{
    int i = profileIndexOf(id);
    if (i < 0) return false;
    if (i != profileActiveIndex() && !profileActivate(i)) return false;
    logDiag("[UI] Profil per PC gewaehlt: %s", profileActive().name);
    deckUiApplyProfile();
    return true;
}

static void cbRoller(lv_event_t*) { switchProfile(lv_roller_get_selected(objects.rollerprofile)); }
static void cbPrev(lv_event_t*)   { int n = profileCount(); if (n > 1) switchProfile((profileActiveIndex() + n - 1) % n); }
static void cbNext(lv_event_t*)   { int n = profileCount(); if (n > 1) switchProfile((profileActiveIndex() + 1) % n); }

// ── Einstellungen: Helligkeit und Design gelten fuer das ganze Geraet ───────────
static void cbBrightness(lv_event_t* e)
{
    int v = lv_arc_get_value(objects.labellightchangevalue);
    if (v < DECK_BRIGHTNESS_MIN) v = DECK_BRIGHTNESS_MIN;
    backlightSet(v);                                        // live mitziehen
    if (lv_event_get_code(e) == LV_EVENT_RELEASED)          // erst beim Loslassen merken
        profilesSetBrightness((uint8_t)v);                  // verzoegert gespeichert (kein sofortiger Flash-Zugriff)
}

static void cbTheme(lv_event_t*)
{
    // PicoPixel hat das Theme in seinem eigenen Callback bereits umgeschaltet (zuerst registriert).
    profilesSetTheme((uint8_t)active_theme_index);          // geraeteweit, verzoegert gespeichert (kein sofortiger Flash-Zugriff)
    styleStatus(usbHostConnected());
    settingsExtrasApplyTheme();
    buildGrid();                                            // leere Plätze folgen der Theme-Farbe
}

// ── Kopfzeile: Uhr und PC-Status ──────────────────────────────────────────────
static void setIfChanged(lv_obj_t* l, const char* txt)
{
    if (strcmp(lv_label_get_text(l), txt) != 0) lv_label_set_text(l, txt);
}

static void diagTickCb(lv_timer_t*)
{
    // alle 10 s eine Zeile auf den UART-Port (nicht in den Ringpuffer, damit er die Ereignisse behaelt)
    char b[320]; LvglFlushStats f;
    lvglPortTakeFlushStats(f);
    logStatus(b, sizeof(b));
    Serial.printf("[Diag] %s | letzte 10 s: Flush n=%u avg=%u us max=%u us max_px=%u, laengste LVGL-Runde %u us\n", b, (unsigned)f.count,
                  (unsigned)(f.count ? f.sumUs / f.count : 0), (unsigned)f.maxUs, (unsigned)f.maxPx, (unsigned)f.loopMaxUs);
}

static void tickCb(lv_timer_t*)
{
    settingsExtrasTick();
    profilesTick();                                         // verzoegertes Speichern der Profilauswahl
    // Nach einem Flash-Schreibzugriff ist die Display-Ansteuerung kurz ausgesetzt (der RGB-Treiber ist nicht IRAM-sicher).
    // Damit nichts Fehlerhaftes stehen bleibt, einmal alles neu zeichnen.
    static uint32_t lastFlash = 0;
    if (diagFlashCounter() != lastFlash) {
        lastFlash = diagFlashCounter();
        lvglPortRestartPanel();                             // Panel-DMA neu starten, falls der LCD-Controller aus dem Takt geraten ist
        lv_obj_invalidate(lv_scr_act());
        logDiag("[UI] Panel neu gestartet und Vollbild neu gezeichnet (nach Flash-Schreibzugriff)");
    }
    LvglSlowFlush sf;                                       // Ausreisser-Flushes ins Log (Ringpuffer), damit sie sich Ereignissen zuordnen lassen
    if (lvglPortTakeSlowFlush(sf))
        logDiag("[Flush] langsam: %u mal, max %u us (%u px bei x=%d y=%d %dx%d)", (unsigned)sf.n, (unsigned)sf.maxUs, (unsigned)sf.px, sf.x, sf.y, sf.w, sf.h);
    char buf[8];
    time_t now = time(nullptr);
    if (now > 1700000000) {                                 // Zeit ist gesetzt (kommt spaeter vom PC)
        struct tm t; localtime_r(&now, &t);
        strftime(buf, sizeof(buf), "%H:%M", &t);
    } else {
        strcpy(buf, "--:--");
    }
    setIfChanged(objects.labeltime, buf);
    const bool up = usbHostConnected();
    static int lastUp = -1;
    if (lastUp != (int)up) { lastUp = (int)up; styleStatus(up); }
    setIfChanged(objects.labelconnectionpc, up ? "PC verbunden" : "Kein PC");
}

// ── Init ──────────────────────────────────────────────────────────────────────
void deckUiBootStatus(const char* text, int percent)
{
    lv_label_set_text(objects.labelstatus, text);
    lv_bar_set_value(objects.barwifi, percent, LV_ANIM_OFF);
}

void deckUiInit()
{
    initStyles();

    // Der Container aus PicoPixel ist nur ein Platzhalter: unsichtbar machen, Kacheln entstehen im Code
    lv_obj_t* g = objects.containergrid;
    lv_obj_clear_flag(g, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(g, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g, 0, 0);
    lv_obj_set_style_outline_width(g, 0, 0);
    lv_obj_set_style_shadow_width(g, 0, 0);
    lv_obj_set_style_radius(g, 0, 0);
    lv_obj_set_style_pad_all(g, 0, 0);

    // Profil-Roller mit den gespeicherten Profilnamen fuellen
    String names;
    profileNamesJoined(names);
    lv_roller_set_options(objects.rollerprofile, names.c_str(), LV_ROLLER_MODE_NORMAL);

    lv_obj_add_event_cb(objects.rollerprofile, cbRoller, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(objects.labelbuttonbackward, cbPrev, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(objects.labelbuttonforward,  cbNext, LV_EVENT_CLICKED, nullptr);
    for (int n = -1; n < DECK_QUICK_COUNT; n++)
        lv_obj_add_event_cb(quickBtn(n), cbQuick, LV_EVENT_CLICKED, (void*)(intptr_t)n);
    lv_obj_add_event_cb(objects.labellightchangevalue, cbBrightness, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(objects.labellightchangevalue, cbBrightness, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(objects.labelswitchtheme, cbTheme, LV_EVENT_CLICKED, nullptr);

    lv_label_set_text(objects.labelversion, "v" FIRMWARE_VERSION);

    settingsExtrasInit();
    deckUiApplyProfile();
    lv_timer_create(tickCb, 1000, nullptr);
    lv_timer_create(diagTickCb, 10000, nullptr);
    tickCb(nullptr);
}
