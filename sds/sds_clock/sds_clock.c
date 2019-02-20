/** @file */ 

#include "sds_clock.h"
#include "app_timer.h"

#ifdef USE_SDS_LOG_CONFIG
#include "sds_log_config.h"
#endif

#define NRF_LOG_MODULE_NAME sds_clk
#if SDS_CLOCK_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_CLOCK_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_CLOCK_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_CLOCK_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();


void sds_get_ms(unsigned long *timestamp) {
	if (timestamp == NULL) return;
	uint32_t count = app_timer_cnt_get();
	
	*timestamp = count  * ( (APP_TIMER_CONFIG_RTC_FREQUENCY + 1 ) * 1000 ) / APP_TIMER_CLOCK_FREQ;
	
	return;
}
