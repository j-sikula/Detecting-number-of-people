#include <avr/io.h>
#include "gpio.h"
#include "platform.h"
#include "delay.h"





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

  delayInit();
  
  VL53L7CX_Platform platform = {.address = 0x52};

  while(1){
    VL53L7CX_WaitMs(&platform, 2000);
    GPIO_toggle(&PORTB, PB5);

  }
  
  
  return 0;
}