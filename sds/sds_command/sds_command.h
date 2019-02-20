/** @file */ 

#ifndef SDS_COMMAND_H_
#define SDS_COMMAND_H_

#include <stdbool.h>
#include <stddef.h>
#include "sds_ble_motion_types.h"
#include "sds_motion_types.h"

/**
    * @brief gets timestamp in ms
    * @param pointer to desired timestamp result 
    * @return none
*/

uint32_t command_init(void);
uint32_t send_serial_response(sds_response_t p_response);
uint32_t send_serial_data(motion_sample_t * p_motion_sample);

#endif
