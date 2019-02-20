#ifndef SDS_SPI_H_
#define SDS_SPI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "sdk_common.h"
#include "nrf_drv_common.h"
#include "nrf_drv_spi.h"


ret_code_t sds_spi_init(void);
ret_code_t sds_spi_transfer(uint8_t* m_tx_buf, uint8_t tx_length, uint8_t* m_rx_buf, uint8_t rx_length);
void sds_spi_event_handler(nrf_drv_spi_evt_t const * p_event, void *p_context);

#endif
