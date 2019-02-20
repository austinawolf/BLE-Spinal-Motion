/** @file */ 

#ifndef CLOCK_INTERFACE_H_
#define CLOCK_INTERFACE_H_

#include <stdbool.h>
#include <stddef.h>

/**
    * @brief gets timestamp in ms
    * @param pointer to desired timestamp result 
    * @return none
*/

void sds_get_ms(unsigned long * count);

#endif
