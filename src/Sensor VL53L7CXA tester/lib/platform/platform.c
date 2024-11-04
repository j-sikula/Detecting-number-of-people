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
#include <stdlib.h>
#include "gpio.h"
#include "delay.h"
#include "twi.h"
#include "uart.h"





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
		uint8_t address = (uint8_t)((p_platform->address >> 1) & 0xFF);		//tested, address 0x29 respond
		status = twi_write((address<<1)|TWI_WRITE);
		

		status = twi_write(RegisterAdress >> 8);
		
		status = twi_write(RegisterAdress & 0xff);
		
		
		for (uint8_t j = 0; j < TWI_BUFFER_SIZE; j++)	// used for filling buffer with data
		{
			if (twi_write(*(p_values + i)) != 0) {
				uart_putc(i+'0');
				uart_puts("NACK write TWI\n");
				delay(100);
				return 1;
			} else {
				i++;
				if (i >= size){
					twi_stop();
					return 0;
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

/**
 * Upload firmware from UART to I2C
 */

uint8_t VL53L7CX_WrFirmware(VL53L7CX_Platform *p_platform,
		uint16_t RegisterAdress,
		char* typeOfRequestedData,		
		uint32_t size)
{
	uint8_t status = 0;
	uart_puts(typeOfRequestedData);	//request firmware from uart
	
	uint8_t address = (uint8_t)((p_platform->address >> 1) & 0xFF);
	twi_start();
	status = twi_write((address<<1)|TWI_WRITE);
		

	status = twi_write(RegisterAdress >> 8);
		
	status = twi_write(RegisterAdress & 0xff);
	uart_puts("Address sent");

	uint8_t bufferCounter = 0;
	unsigned int receivedChar = 0;
	for(uint32_t i = 0; i < size; i++)
	{
		receivedChar = uart_getc();		
		while ((receivedChar & 0xff00) != 0)	//wait for data
		{
			receivedChar = uart_getc();			
		}
		/*
		char string[8];
		itoa(bufferCounter, string, 10);
		uart_puts(string);
		uart_puts("buffCount\nReceived char");
		uart_putc((receivedChar & 0xFF)+'0');
		uart_putc((receivedChar >>8)+'0');
		uart_puts("\n");

		*/

		if (bufferCounter < 30)
		{		
			status |= twi_write(receivedChar & 0xFF);
			bufferCounter++;
		}	
		else
		{
			bufferCounter = 0;
			status |= twi_write(receivedChar & 0xFF);
			
			if (status == 0){
			uart_putc('C'); //requesting for other packet
			}
			else
			{
				uart_puts("Status wrong C");
			}
		}
	}
	status = 0;
	twi_stop();
	uart_puts("\nSize of uploaded: ");
	char string[8];
	itoa(size, string, 10);
	uart_puts(string);
	uart_puts("\n");

	return status;
}

