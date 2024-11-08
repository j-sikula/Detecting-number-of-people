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
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_FREQ_HZ 100000
#define BUILT_IN_LED 2



/*-----------------------------------------------------------*/
// Used function(s)
void vTaskI2CScanner();
void vTaskLoop();


/*-----------------------------------------------------------*/
/* In ESP-IDF instead of "main", we use "app_main" function
   where the program execution begins */
void app_main(void)
{
    //Set GPIO
    gpio_reset_pin(BUILT_IN_LED); 
    gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);

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

	/* (Optional) Reset sensor toggling PINs (see platform, not in API) */
	//VL53L7CX_Reset_Sensor(&(Dev.platform));

	/* (Optional) Set a new I2C address if the wanted address is different
	* from the default one (filled with 0x20 for this example).
	*/
	//status = vl53l7cx_set_i2c_address(&Dev, 0x20);

	
	/*********************************/
	/*   Power on sensor and init    */
	/*********************************/

    VL53L7CX_InitCommunication(&(Dev.platform));

	/* (Optional) Check if there is a VL53L7CX sensor connected */
	status = vl53l7cx_is_alive(&Dev, &isAlive);
            


    ESP_LOGI("setup", "i2c scan application");

    // Configure i2c controller in master mode
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    ESP_LOGI("i2c", "i2c controller configured");

    // Install i2c driver
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    ESP_LOGI("i2c", "i2c driver installed");

    // Start the i2c scanner task
    xTaskCreate(vTaskI2CScanner, "i2c_scanner", 2048, NULL, 5, NULL);
}


/*-----------------------------------------------------------*/
void vTaskI2CScanner()
{
    uint8_t devices_found = 0;

    ESP_LOGI("i2c", "scanning the bus...");
    uint8_t sla = 29;
        // Create the i2c commands list
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Queue a "START" signal to a list
    i2c_master_start(cmd);

    // Slave address to write data
    i2c_master_write_byte(cmd, (sla<<1) | I2C_MASTER_WRITE, true);

    // Queue a "STOP" signal to a list
    i2c_master_stop(cmd);

    ESP_LOGI("i2c", "0x%02x", sla);

    // Send all the queued commands on the I2C bus, in master mode
    if (i2c_master_cmd_begin(I2C_NUM_0, cmd, (1000/portTICK_PERIOD_MS)) == ESP_OK) {
        ESP_LOGI("i2c", "found device with address 0x%02x", sla);
        devices_found++;
    }

    // Free the I2C commands list
    i2c_cmd_link_delete(cmd);

    vTaskDelay(25 / portTICK_PERIOD_MS);  // 25 milliseconds
    
    ESP_LOGI("i2c", "...scan completed!");
    ESP_LOGI("i2c", "%d device(s) found", devices_found);

    // Start the loop task
    xTaskCreate(vTaskLoop, "forever_loop", 2048, NULL, 5, NULL);

    // Delete this task
    vTaskDelete(NULL);
}


/*-----------------------------------------------------------*/
void vTaskLoop()
{
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