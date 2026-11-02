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

void backlightPower(bool on) { IO_EXTENSION_Output(IO_EXTENSION_IO_2, on ? 1 : 0); }   // IO2 = Backlight-Enable (siehe rgb_lcd_port.cpp)

void backlightInit() { backlightSet(s_percent); }
