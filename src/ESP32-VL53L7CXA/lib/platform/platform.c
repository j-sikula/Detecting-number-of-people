/**
 *
 * Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "platform.h"
#include <esp_log.h> // ESP_LOG/E/W/I functions
#include <driver/i2c.h>

void VL53L7CX_InitCommunication(VL53L7CX_Platform *p_platform)
{
	gpio_reset_pin(INT);
	gpio_reset_pin(I2C_RST);
	gpio_reset_pin(LPN);
	gpio_reset_pin(PWR_EN);

	gpio_set_direction(INT, GPIO_MODE_INPUT);
	gpio_set_direction(I2C_RST, GPIO_MODE_OUTPUT);
	gpio_set_direction(LPN, GPIO_MODE_OUTPUT);
	gpio_set_direction(PWR_EN, GPIO_MODE_OUTPUT);

	gpio_set_level(I2C_RST, 0); // no reset
	gpio_set_level(LPN, 1);		// no LPn
	gpio_set_level(PWR_EN, 1);	// enabled

	ESP_LOGI("setup", "i2c communication init");

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
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	cmd = i2c_cmd_link_create();

	// Queue a "START" signal to a list
	i2c_master_start(cmd);

	// Slave address to write data
	i2c_master_write_byte(cmd, (p_platform->address) | I2C_MASTER_WRITE, true);

	// Queue a "STOP" signal to a list
	i2c_master_stop(cmd);

	ESP_LOGI("i2c", "0x%02x", p_platform->address);
	
	// Send all the queued commands on the I2C bus, in master mode
	if (i2c_master_cmd_begin(I2C_NUM_0, cmd, (1000 / portTICK_PERIOD_MS)) == ESP_OK)
	{		
		ESP_LOGI("i2c", "found device with address 0x%02x", p_platform->address);
	}

	// Free the I2C commands list
	i2c_cmd_link_delete(cmd);
}

uint8_t VL53L7CX_RdByte(
	VL53L7CX_Platform *p_platform,
	uint16_t RegisterAdress,
	uint8_t *p_value)
{

	uint8_t status = VL53L7CX_RdMulti(p_platform, RegisterAdress, p_value, 1);

	return status;
}

uint8_t VL53L7CX_WrByte(
	VL53L7CX_Platform *p_platform,
	uint16_t RegisterAdress,
	uint8_t value)
{

	uint8_t status = VL53L7CX_WrMulti(p_platform, RegisterAdress, &value, 1);

	return status;
}

uint8_t VL53L7CX_WrMulti(
	VL53L7CX_Platform *p_platform,
	uint16_t RegisterAdress,
	uint8_t *p_values,
	uint32_t size)
{
	uint8_t status = 255;

	// https://github.com/stm32duino/VL53L7CX/blob/main/src/platform.cpp
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	ESP_LOGI("i2c", "Writing %" PRIu32 " bytes", size);
	uint32_t i = 0;
	while (i < size)
	{
		
		
		// Queue a "START" signal to a list
		i2c_master_start(cmd);

		// Slave address to write data
		i2c_master_write_byte(cmd, (p_platform->address) | I2C_MASTER_WRITE, 0);
		i2c_master_write_byte(cmd, RegisterAdress >> 8, 0);
		i2c_master_write_byte(cmd, RegisterAdress & 0xff, true);
		

		// Send all the queued commands on the I2C bus, in master mode
		if (i2c_master_cmd_begin(I2C_NUM_0, cmd, (1000 / portTICK_PERIOD_MS)) == ESP_OK)
		{
			ESP_LOGI("i2c", "Address sent");
		}
		

		for (uint8_t j = 0; j < I2C_BUFFER_SIZE; j++) // used only for filling buffer with data, var j not used
		{
			if (i < (size-1))
			{
				i2c_master_write_byte(cmd, *(p_values + i), false);
				i++;
			}
			else
			{
				i2c_master_write_byte(cmd, *(p_values + i), true);
				i++;				
				break;
				
			}
		}
		// Queue a "STOP" signal to a list
		i2c_master_stop(cmd);

		// Send all the queued commands on the I2C bus, in master mode
		if (i2c_master_cmd_begin(I2C_NUM_0, cmd, (1000 / portTICK_PERIOD_MS)) == ESP_OK)
		{
			ESP_LOGI("i2c", "Packet sent");
			status = 0;
		}
	}

	

	/* Need to be implemented by customer. This function returns 0 if OK */

	return status;
}

uint8_t VL53L7CX_RdMulti(
	VL53L7CX_Platform *p_platform,
	uint16_t RegisterAdress,
	uint8_t *p_values,
	uint32_t size)
{
	uint8_t status = 255;

	// https://github.com/stm32duino/VL53L7CX/blob/main/src/platform.cpp
	/*
		uint32_t i = 0;
		while (i < size)
		{
			twi_start();
			uint8_t address = (uint8_t)((p_platform->address >> 1) & 0xFF);
			status = twi_write(address<<1|TWI_WRITE);

			status = twi_write(RegisterAdress >> 8);
			status = twi_write(RegisterAdress & 0xff);

			twi_stop();
			twi_start();

			status = twi_write(address<<1|TWI_READ);

			for (uint8_t j = 0; j < TWI_BUFFER_SIZE; j++)	// used for filling buffer with data
			{

				if(size-i <= 1){	//last byte with NACK
					p_values[i] = twi_read(TWI_NACK);
					twi_stop();
					return 0;
				}
				else
				{
					p_values[i] = twi_read(TWI_ACK);
					i++;
				}
			}
		}
		twi_stop();
		status = 0;



		*/
	return status;
}
/**
 * should reset sensor, now it pulls down SDA line
 */

uint8_t VL53L7CX_Reset_Sensor(
	VL53L7CX_Platform *p_platform)
{
	uint8_t status = 0;

	/* (Optional) Need to be implemented by customer. This function returns 0 if OK */

	/* Set pin LPN to LOW */
	gpio_set_level(LPN, 0);
	/* Set pin AVDD (PWR_EN) to LOW */
	gpio_set_level(PWR_EN, 0);
	/* Set pin VDDIO  to LOW */
	VL53L7CX_WaitMs(p_platform, 100);

	/* Set pin LPN of to HIGH */
	gpio_set_level(LPN, 1);
	/* Set pin AVDD of to HIGH */
	gpio_set_level(PWR_EN, 0);
	/* Set pin VDDIO of  to HIGH */
	VL53L7CX_WaitMs(p_platform, 100);

	return status;
}

void VL53L7CX_SwapBuffer(
	uint8_t *buffer,
	uint16_t size)
{
	uint32_t i, tmp;

	/* Example of possible implementation using <string.h> */
	for (i = 0; i < size; i = i + 4)
	{
		tmp = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) | (buffer[i + 3]);

		memcpy(&(buffer[i]), &tmp, 4);
	}
}

uint8_t VL53L7CX_WaitMs(
	VL53L7CX_Platform *p_platform,
	uint32_t TimeMs)
{

	/* Need to be implemented by customer. This function returns 0 if OK */
	vTaskDelay(TimeMs / portTICK_PERIOD_MS);

	return 0;
}