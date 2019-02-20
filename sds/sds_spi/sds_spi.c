
#include "sds_spi.h"
#include "nrf_gpio.h"
#include "boards.h"

#define NRF_LOG_MODULE_NAME sds_spi

#define SDS_SPI_CONFIG_LOG_ENABLED 1
#define SDS_SPI_CONFIG_LOG_LEVEL 4
#define SDS_SPI_CONFIG_INFO_COLOR 0
#define SDS_SPI_CONFIG_DEBUG_COLOR 0

#if SDS_SPI_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       SDS_SPI_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR  SDS_SPI_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_SPI_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */
   

ret_code_t sds_spi_init(void) {
	ret_code_t ret;	
	
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
	spi_config.frequency = NRF_DRV_SPI_FREQ_4M;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
	
    ret = nrf_drv_spi_init(&spi, &spi_config, sds_spi_event_handler, NULL);
	return ret;
}

void sds_spi_event_handler(nrf_drv_spi_evt_t const * p_event, void *p_context)
{
	spi_xfer_done = true;
    NRF_LOG_DEBUG("Transfer completed.");
}

ret_code_t sds_spi_transfer(uint8_t* m_tx_buf, uint8_t tx_length, uint8_t* m_rx_buf, uint8_t rx_length) {
	
	ret_code_t ret;
	
	//setup temp buffer
	uint8_t m_rx_buf_temp[rx_length+1];
	memset(m_rx_buf_temp, 0, rx_length+1);
	
	//start transfer
	spi_xfer_done = false;
	
	if (rx_length != 0x0) {
		ret = nrf_drv_spi_transfer(&spi, m_tx_buf, tx_length, m_rx_buf_temp, rx_length+1);
	}
	else {
		ret = nrf_drv_spi_transfer(&spi, m_tx_buf, tx_length, 0, 0);
	}
	
	//wait
	while (!spi_xfer_done)
	{
			__WFE();
	}
	
	//copy temp buffer to original buffer
	if (m_rx_buf != 0x0) {
		memcpy(m_rx_buf, &m_rx_buf_temp[1], rx_length);
	}	
	
	return ret;
}
