#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpio.h"
#include "platform.h"
#include "delay.h"
#include "twi.h"
#include "vl53l7cx_api.h"
#include "uart.h"





int main(void){
  
  GPIO_mode_input_nopull(&PORTD, INT);
  GPIO_mode_output(&PORTD, I2C_RST);
  GPIO_mode_output(&PORTD, LPN);  
  GPIO_mode_output(&PORTD, PWR_EN);    
  GPIO_mode_output(&PORTB, PB5);

  GPIO_write_low(&PORTD, I2C_RST);  //no reset
  GPIO_write_high(&PORTD, LPN);     //no LPn
  GPIO_write_high(&PORTD, PWR_EN);  //enabled

  GPIO_write_low(&PORTB, PB5);
  //Mandatory init commands for good library working
  delayEnable();  //Needed for using delay in platform.c
  twi_init();   //I2C init
  uart_init((unsigned int)115200);
  
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

	/* (Optional) Check if there is a VL53L7CX sensor connected */
	status = vl53l7cx_is_alive(&Dev, &isAlive);
	if(!isAlive || status)
	{
		uart_puts("VL53L7CX not detected at requested address\n");
		return status;
	}

	/* (Mandatory) Init VL53L7CX sensor */
	status = vl53l7cx_init(&Dev);
	if(status)
	{
		uart_puts("VL53L7CX ULD Loading failed\n");
		return status;
	}

	uart_puts("VL53L7CX ULD ready ! \n");
			
	/*********************************/
	/*     Change the power mode     */
	/*********************************/

	/* For the example, we don't want to use the sensor during 10 seconds. In order to reduce
	 * the power consumption, the sensor is set to low power mode.
	 */
	status = vl53l7cx_set_power_mode(&Dev, VL53L7CX_POWER_MODE_SLEEP);
	if(status)
	{
		uart_puts("vl53l7cx_set_power_mode failed, status not 0\n");
		return status;
	}
	uart_puts("VL53L7CX is now sleeping\n");

	/* We wait 5 seconds, only for the example */
	uart_puts("Waiting 5 seconds for the example...\n");
	VL53L7CX_WaitMs(&(Dev.platform), 5000);

	/* After 5 seconds, the sensor needs to be restarted */
	status = vl53l7cx_set_power_mode(&Dev, VL53L7CX_POWER_MODE_WAKEUP);
	if(status)
	{
		uart_puts("vl53l7cx_set_power_mode failed, status not 0\n");
		return status;
	}
	uart_puts("VL53L7CX is now waking up\n");

	/*********************************/
	/*         Ranging loop          */
	/*********************************/

	status = vl53l7cx_start_ranging(&Dev);

	loop = 0;
	while(loop < 10)
	{
		/* Use polling function to know when a new measurement is ready.
		 * Another way can be to wait for HW interrupt raised on PIN A3
		 * (GPIO 1) when a new measurement is ready */
 
		status = vl53l7cx_check_data_ready(&Dev, &isReady);

		if(isReady)
		{
			vl53l7cx_get_ranging_data(&Dev, &Results);

			/* As the sensor is set in 4x4 mode by default, we have a total 
			 * of 16 zones to print. For this example, only the data of first zone are 
			 * print */
      char string[3];
      uart_puts("Print data no : \n");
      itoa(Dev.streamcount, string, 10);
			uart_puts(string);
			for(i = 0; i < 16; i++)
			{
        
				uart_puts("Zone : ");
				       
        itoa(i, string, 10);
        uart_puts(string);
        uart_puts("Status : ");
        itoa(Results.target_status[VL53L7CX_NB_TARGET_PER_ZONE*i], string, 10);
        uart_puts(string);
        uart_puts("Distance : ");
        itoa(Results.distance_mm[VL53L7CX_NB_TARGET_PER_ZONE*i], string, 10);
        uart_puts(string);
        uart_puts(" mm \n");
			}
			uart_puts("\n");
			loop++;
		}

		/* Wait a few ms to avoid too high polling (function in platform
		 * file, not in API) */
		VL53L7CX_WaitMs(&(Dev.platform), 5);
	}

	status = vl53l7cx_stop_ranging(&Dev);
	uart_puts("End of ULD demo\n");
	return status;
  

  while(1){
    if(twi_test_address(0x52)){
      GPIO_write_high(&PORTB, PB5);
    }
    

  }
  
  
  return 0;
}


