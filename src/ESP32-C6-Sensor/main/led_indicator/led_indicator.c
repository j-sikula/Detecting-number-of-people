#include "led_indicator.h"
#include "sdkconfig.h"

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static uint8_t red_led_intensity = 16;
static uint8_t green_led_intensity = 16;
static uint8_t blue_led_intensity = 16;



void blink_led(void)
{
	/* If the addressable LED is enabled */
	if (s_led_state)
	{
		/* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
		led_strip_set_pixel(led_strip, 0, red_led_intensity, green_led_intensity, blue_led_intensity);
		/* Refresh the strip to send data */
		led_strip_refresh(led_strip);
	}
	else
	{
		/* Set all LED off to clear all pixels */
		led_strip_clear(led_strip);
	}
}

void led_loop(void)
{
    while (1)
	{
		// ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
		blink_led();
		/* Toggle the LED state */
		s_led_state = !s_led_state;
		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
	}
}

void stop_led(void)
{
    red_led_intensity = 0;
    green_led_intensity = 0;    
    blue_led_intensity = 0;
}

void configure_led(void)
{
	/* LED strip initialization with the GPIO and pixels number*/
	led_strip_config_t strip_config = {
		.strip_gpio_num = BLINK_GPIO,
		.max_leds = 1, // at least one LED on board
	};
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
	led_strip_rmt_config_t rmt_config = {
		.resolution_hz = 10 * 1000 * 1000, // 10MHz
		.flags.with_dma = false,
	};
	ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#endif
	/* Set all LED off to clear all pixels */
	led_strip_clear(led_strip);
}

#endif