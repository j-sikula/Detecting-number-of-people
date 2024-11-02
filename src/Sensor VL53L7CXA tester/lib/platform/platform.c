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
#include <avr/io.h>
#include "gpio.h"
#include "delay.h"
#include "twi.h"



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

	//https://github.com/stm32duino/VL53L7CX/blob/main/src/platform.cpp

	uint32_t i = 0;
	while (i < size) 
	{
		twi_start();
		uint8_t address = (uint8_t)((p_platform->address << 1) & 0xFF);
		twi_write(address|TWI_WRITE);

		twi_write(RegisterAdress >> 8);
		twi_write(RegisterAdress & 0xff);
		
		for (uint8_t j = 0; j < TWI_BUFFER_SIZE; j++)	// used for filling buffer with data
		{
			if (twi_write(p_values + i) == 0) {
				return 1;
			} else {
				i ++;
				if (i >= size){
					break;
				}
			}
		}
		
    }
	twi_stop();
	status = 0;	
   
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

	//https://github.com/stm32duino/VL53L7CX/blob/main/src/platform.cpp

	uint32_t i = 0;
	while (i < size) 
	{
		twi_start();
		uint8_t address = (uint8_t)((p_platform->address << 1) & 0xFF);
		twi_write(address|TWI_WRITE);

		twi_write(RegisterAdress >> 8);
		twi_write(RegisterAdress & 0xff);
		
		twi_stop();
		twi_start();

		twi_write(address|TWI_READ);
		
		for (uint8_t j = 0; j < TWI_BUFFER_SIZE; j++)	// used for filling buffer with data
		{
			p_values[i] = twi_read(TWI_ACK);
        	i++;
			if(size-i == 1){	//last byte with NACK
				p_values[i] = twi_read(TWI_NACK);
				i++;
				break;
			}
		}
    }
	twi_stop();
	status = 0;	

	
	/* Need to be implemented by customer. This function returns 0 if OK */
	
	return status;
}

uint8_t VL53L7CX_Reset_Sensor(
		VL53L7CX_Platform *p_platform)
{
	uint8_t status = 0;
	
	/* (Optional) Need to be implemented by customer. This function returns 0 if OK */
	
	/* Set pin LPN to LOW */
	GPIO_write_low(&PORTD, LPN);
	/* Set pin AVDD to LOW */
	GPIO_write_low(&PORTD, PWR_EN);
	/* Set pin VDDIO  to LOW */
	VL53L7CX_WaitMs(p_platform, 100);

	/* Set pin LPN of to HIGH */
	GPIO_write_high(&PORTD, LPN);
	/* Set pin AVDD of to HIGH */
	GPIO_write_high(&PORTD, PWR_EN);
	/* Set pin VDDIO of  to HIGH */
	VL53L7CX_WaitMs(p_platform, 100);

	return status;
}

void VL53L7CX_SwapBuffer(
		uint8_t 		*buffer,
		uint16_t 	 	 size)
{
	uint32_t i, tmp;
	
	/* Example of possible implementation using <string.h> */
	for(i = 0; i < size; i = i + 4) 
	{
		tmp = (
		  buffer[i]<<24)
		|(buffer[i+1]<<16)
		|(buffer[i+2]<<8)
		|(buffer[i+3]);
		
		memcpy(&(buffer[i]), &tmp, 4);
	}
}	

uint8_t VL53L7CX_WaitMs(
		VL53L7CX_Platform *p_platform,
		uint32_t TimeMs)
{
	
	/* Need to be implemented by customer. This function returns 0 if OK */
	delay(TimeMs);
	
	return 0;
}
