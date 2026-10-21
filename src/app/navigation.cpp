#include "navigation.h"

// Navigationsplan:
//   Deck (screen_1) <-> Einstellungen (screensetting)   über Zahnrad bzw. Home-Button
//   Einstellungen -> Rechner / Timer (entstehen im Code, kehren zu den Einstellungen zurück)

void nav_goto(enum ScreensEnum id)
{
    lv_obj_t *scr = nullptr;
    switch (id) {
        case SCREEN_ID_SCREENSETTING: scr = objects.screensetting; break;
        case SCREEN_ID_LAGERSCREEN:   scr = objects.lagerscreen;   break;
        case SCREEN_ID_SCREEN_1:      scr = objects.screen_1;      break;
        case SCREEN_ID_SCREENBOOT:    scr = objects.screenboot;    break;
    }
    if (scr) lv_scr_load(scr);
}

// Ein Callback für alle Ziele: das Ziel steckt im user_data
static void on_nav(lv_event_t *e)
{
    nav_goto((enum ScreensEnum)(intptr_t)lv_event_get_user_data(e));
}

static void link(lv_obj_t *btn, enum ScreensEnum target)
{
    if (!btn) return;
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, on_nav, LV_EVENT_CLICKED, (void *)(intptr_t)target);
}

void nav_init()
{
    link(objects.buttonsetting,  SCREEN_ID_SCREENSETTING);   // Deck -> Einstellungen
    link(objects.labelbuttonhome, SCREEN_ID_SCREEN_1);       // Einstellungen -> Deck
}
