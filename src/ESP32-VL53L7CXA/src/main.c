/*
   The I2C bus scanner tests all addresses and detects chips and
   modules that are connected to the SDA and SCL signals.
   Xtensa dual-core 32-bit LX6 (ESP32-CAM), 240 MHz
   PlatformIO, ESP-IDF framework

   Copyright (c) 2022 Tomas Fryza, 2024 Josef Sikula
   Dept. of Radio Electronics, Brno University of Technology, Czechia
   This work is licensed under the terms of the GNU GENERAL PUBLIC LICENSE.
 */


/*-----------------------------------------------------------*/
#include <freertos/FreeRTOS.h>  // FreeRTOS
#include <freertos/task.h>
#include <esp_log.h>            // ESP_LOG/E/W/I functions
#include <driver/i2c.h>         // Inter-Integrated Circuit driver
#include "platform.h"
#include "vl53l7cx_api.h"

/*-----------------------------------------------------------*/

#define BUILT_IN_LED 2



/*-----------------------------------------------------------*/
// Used function(s)

void vTaskLoop();


/*-----------------------------------------------------------*/
/* In ESP-IDF instead of "main", we use "app_main" function
   where the program execution begins */
void app_main(void)
{
    //Set GPIO
    gpio_reset_pin(BUILT_IN_LED); 
    gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);       

    // Start the i2c scanner task
    xTaskCreate(vTaskLoop, "forever_loop", 40*1024, NULL, 5, NULL);
}




/*-----------------------------------------------------------*/
void vTaskLoop()
{
     
    /*********************************/
	/*   VL53L7CX ranging variables  */
	/*********************************/

	uint8_t 				status, loop, isAlive, isReady, i;
	VL53L7CX_Configuration 	Dev;			/* Sensor configuration */
	VL53L7CX_ResultsData 	Results;		/* Results data from VL53L7CX */
    

	/*********************************/
	/*      Customer platform        */
	/*********************************/

	/* Fill the platform structure with customer's implementation. For this
	* example, only the I2C address is used.
	*/
	Dev.platform.address = VL53L7CX_DEFAULT_I2C_ADDRESS;

    VL53L7CX_InitCommunication(&(Dev.platform));

	/* (Optional) Reset sensor toggling PINs (see platform, not in API) */
	//VL53L7CX_Reset_Sensor(&(Dev.platform));

		
	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

   
    ESP_LOGI("setup", "Memory allocated");

	/* (Optional) Check if there is a VL53L7CX sensor connected */
	status = vl53l7cx_is_alive(&Dev, &isAlive);
    if(!isAlive || status)
	{
		ESP_LOGI("sensor","VL53L7CX not detected at requested address\n");
		
	}
   
    status = vl53l7cx_init(&Dev);
	if(status)
	{
		ESP_LOGI("sensor","VL53L7CX ULD Loading failed\n");
		
	}

	ESP_LOGI("sensor","VL53L7CX ULD ready ! (Version : %s)\n",
			VL53L7CX_API_REVISION);
    
    
    // Forever loop
    while (1) {
        gpio_set_level(BUILT_IN_LED, 1);        // Set high level
        vTaskDelay(100 / portTICK_PERIOD_MS);   // Delay 100 milliseconds

        gpio_set_level(BUILT_IN_LED, 0);        // Set low level
        vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay 1 second
    }

    // Delete this task if it exits from the loop above
    vTaskDelete(NULL);
}