#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "led_strip.h"
#include <stdio.h>

void configure_led(void);
void blink_led(void);
void led_loop(void);

void stop_led(void);

#endif // LED_INDICATOR_H