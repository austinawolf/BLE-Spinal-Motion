#ifndef TWI_INTERFACE_H_
#define TWI_INTERFACE_H_


#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "sdk_common.h"
#include "nrf_drv_common.h"
#include "app_util_platform.h"

void sds_twi_init (void);
int sds_twi_scan (void);
int sds_twi_write(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char const *data);
int sds_twi_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data);
int sds_write_read(uint8_t address, uint8_t command);
int sds_write_write(uint8_t address, uint8_t command, uint8_t data);

#endif
