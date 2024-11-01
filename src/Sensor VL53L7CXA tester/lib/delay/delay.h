
#ifndef DELAY_H
# define DELAY_H

#include <stdint.h>
/**
 * Enables Timer0 and for counting milliseconds
 */
void delayInit();


/**
 * @ms stops programme for  
 *  
 */
void delay(uint32_t ms);


#endif