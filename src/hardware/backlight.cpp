#include "backlight.h"
#include <Arduino.h>
#include "io_extension.h"
#include "../app/app_config.h"

static uint8_t s_percent = DECK_BRIGHTNESS_DEFAULT;

void backlightSet(uint8_t percent)
{
  s_percent = constrain(percent, DECK_BRIGHTNESS_MIN, 100);
  // Der Expander-PWM ist invertiert (0 = hell, 97 = dunkel).
  IO_EXTENSION_Pwm_Output((uint8_t)(100 - s_percent));
}

uint8_t backlightGet() { return s_percent; }

void backlightInit() { backlightSet(s_percent); }
