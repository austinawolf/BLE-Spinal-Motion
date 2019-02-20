/** @file */ 

#include "sds_command.h"

#define NRF_LOG_MODULE_NAME cmd
#if SDS_CMD_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_CMD_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_CMD_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_CMD_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();



