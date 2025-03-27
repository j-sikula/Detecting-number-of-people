#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "led_strip.h"
#include <stdio.h>

/**
 * Minimum blink period is 100 ms to allow using uint8_t
 */
#define BASE_PERIOD 100 
#define DEFAULT_BLINK_INTENSITY 16

void configure_led(void);
void blink_led(uint8_t blink_period);
void led_loop(void);

void stop_led(void);

/**
 * @brief Set the color of the LED for a single blink
 * @param duration_100ms duration of the single blink - multiplication of 100 ms
 */
void single_blink(uint8_t red, uint8_t green, uint8_t blue, uint8_t duration_100ms);

void set_led_color(uint8_t red, uint8_t green, uint8_t blue);

void set_blink_period(uint8_t period);

#endif // LED_INDICATOR_H