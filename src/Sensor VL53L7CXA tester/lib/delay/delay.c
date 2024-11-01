#include "delay.h"
#include "timer.h"
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC

volatile uint32_t nMillis = 0;


void delayInit()
{
    TIM0_ovf_enable();
    TIM0_ovf_1ms();
    sei();   

}

void delay(uint32_t ms)
{
    nMillis = 0;
    while (nMillis < ms){

    }
    return;
}

ISR(TIMER0_OVF_vect)
{
    TCNT0 = 6;      //init value set to 6 to change tiome of overflow to exactly 1 ms
    nMillis++;
}