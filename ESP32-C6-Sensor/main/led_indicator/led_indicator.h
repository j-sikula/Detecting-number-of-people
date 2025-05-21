#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "led_strip.h"
#include <stdio.h>

/**
 * Minimum blink period is 100 ms to allow using uint8_t
 */
#define BASE_PERIOD 100 
#define DEFAULT_BLINK_INTENSITY 16

/**
 * @brief Initializes the built-in LED
 */
void configure_led(void);

/**
 * @brief When called in infinite loop, blinks the LED with period defined by set_blink_period and color defined by set_led_color
 * @param blink_period period of blinking in multiplication of 100 ms
 */
void blink_led(uint8_t blink_period);

/**
 * @brief LED task, infinite loop
 * @details executes blink_led function and performs single blink (when blink_period is 0, single blink is immediate, else it is delayed)
 */
void led_loop(void);

/**
 * @brief Stop the LED blinking and turn LED off
 */
void stop_led(void);

/**
 * @brief Set the color of the LED for a single blink
 * @param duration_100ms duration of the single blink - multiplication of 100 ms
 */
void single_blink(uint8_t red, uint8_t green, uint8_t blue, uint8_t duration_100ms);

/**
 * @brief Set the color of the LED for periodic blinking
 * @param red red color intensity (0-255)
 * @param green green color intensity (0-255)
 * @param blue blue color intensity (0-255)
 */
void set_led_color(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Set the period of blinking
 * @param period period of blinking in multiplication of 100 ms
 */
void set_blink_period(uint8_t period);

#endif // LED_INDICATOR_H