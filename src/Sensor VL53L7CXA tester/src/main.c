#include <avr/io.h>
#include "gpio.h"
#include "platform.h"




int main(void){

  GPIO_mode_input_nopull(&PORTD, INT);
  GPIO_mode_output(&PORTD, I2C_RST);
  GPIO_mode_output(&PORTD, LPN);  
  GPIO_mode_output(&PORTD, PWR_EN);    

  GPIO_write_low(&PORTD, I2C_RST);  //no reset
  GPIO_write_high(&PORTD, LPN);     //no LPn
  GPIO_write_high(&PORTD, PWR_EN);  //enabled


  while(1){


  }
  
  
  return 0;
}