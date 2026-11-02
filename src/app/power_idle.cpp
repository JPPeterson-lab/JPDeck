#include "power_idle.h"
#include <Arduino.h>
#include "profiles.h"
#include "../hardware/backlight.h"
#include "diag_log.h"

enum IdleState { IDLE_ACTIVE, IDLE_DIM, IDLE_OFF };
static IdleState s_state = IDLE_ACTIVE;
static uint32_t  s_lastActivity = 0;
static bool      s_swallow = false;          // Beruehrung, die das Display geweckt hat, bis zum Loslassen verschlucken
static bool      s_suppress = false;

bool idleDimmed() { return s_state != IDLE_ACTIVE; }

static void restore()
{
  if (s_state == IDLE_OFF) backlightPower(true);
  backlightSet(profilesBrightness());
  s_state = IDLE_ACTIVE;
}

void idleWake()
{
  s_lastActivity = millis();
  if (s_state != IDLE_ACTIVE) { restore(); logDiag("[Idle] Display geweckt"); }
}

void idleSuppress(bool on) { s_suppress = on; if (on) idleWake(); }

bool idleFilterTouch(bool pressed)
{
  if (pressed) {
    s_lastActivity = millis();
    if (s_state != IDLE_ACTIVE) { restore(); s_swallow = true; logDiag("[Idle] Display per Touch geweckt"); }
  } else {
    s_swallow = false;
  }
  return !s_swallow;
}

void idleTick()
{
  if (s_state != IDLE_ACTIVE || s_suppress) return;
  const DeckIdleSettings is = profilesIdle();
  if (is.sec == 0) return;
  if ((uint32_t)(millis() - s_lastActivity) < (uint32_t)is.sec * 1000UL) return;
  if (is.dim == 0) { backlightPower(false); s_state = IDLE_OFF; logDiag("[Idle] Display aus nach %u s", (unsigned)is.sec); }
  else { backlightSet(is.dim); s_state = IDLE_DIM; logDiag("[Idle] Display auf %u %% nach %u s", (unsigned)is.dim, (unsigned)is.sec); }
}
