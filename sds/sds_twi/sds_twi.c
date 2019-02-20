#include "sds_twi.h"

//std
#include <stdio.h>

//nrf
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"

#include "sds_log_config.h"
#define NRF_LOG_MODULE_NAME sds_twi
#if SDS_TWI_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_TWI_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_TWI_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_TWI_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();


/* TWI instance ID. */
#define TWI_INSTANCE_ID     0

 /* Number of possible TWI addresses. */
 #define TWI_ADDRESSES      127

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);



/**
 * @brief TWI initialization.
 */
void sds_twi_init(void)
{
    ret_code_t err_code;

	
    const nrf_drv_twi_config_t twi_config = {
       .scl                = MPU9150_TWI_SCL,
       .sda                = MPU9150_TWI_SDA,
       .frequency          = NRF_DRV_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}


/**
 * @brief Scan for TWI devices
 */

int sds_twi_scan(void)
{
	ret_code_t err_code;
    uint8_t address;
    uint8_t sample_data;

    for (address = 1; address <= TWI_ADDRESSES; address++)
    {
        err_code = nrf_drv_twi_rx(&m_twi, address, &sample_data, sizeof(sample_data));
        if (err_code == NRF_SUCCESS)
        {
			return address;
        }
    }
		return -1;
	
}

int sds_twi_write(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char const *data) {	
	ret_code_t err_code;
	unsigned char out[length+1];
	out[0] = reg_addr;
	memcpy(&out[1],data,length);
	
		
	err_code = nrf_drv_twi_tx(&m_twi, slave_addr, out, length+1, false);
	if (err_code != NRF_SUCCESS) return err_code;  
	
	return NRF_SUCCESS;
}

int sds_twi_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data) {
	ret_code_t err_code;
		
	err_code = nrf_drv_twi_tx(&m_twi, slave_addr, (const uint8_t*) &reg_addr, 1, false);
	if (err_code != NRF_SUCCESS) return err_code;  
	
	err_code = nrf_drv_twi_rx(&m_twi, slave_addr, data, length);							
	if (err_code != NRF_SUCCESS) return err_code;
  	
	return 0;
}





// Old twi functions

int sds_write_read(uint8_t address, uint8_t command)
{
	  ret_code_t err_code;
    uint8_t sample_data;

	
		err_code = nrf_drv_twi_tx(&m_twi, address, &command, 1, true);
		if (err_code != NRF_SUCCESS)
		{
			return -1;
		}	
		err_code = nrf_drv_twi_rx(&m_twi, address, &sample_data, 1);							
		if (err_code == NRF_SUCCESS)
		{
			return sample_data;
		}   
		return -1;
	
}

int sds_write_write(uint8_t address, uint8_t command, uint8_t data) 
{
	  ret_code_t err_code;

		err_code = nrf_drv_twi_tx(&m_twi, address, &command, 1, true);
		if (err_code != NRF_SUCCESS)
		{
			return -1;
		}
		err_code = nrf_drv_twi_tx(&m_twi, address, &data, 1, true);
		if (err_code != NRF_SUCCESS)
		{
			return -1;
		}			   
		return 0;
	
}
