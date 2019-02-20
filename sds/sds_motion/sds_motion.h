#ifndef SDS_MOTION_H_
#define SDS_MOTION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "sdk_common.h"
#include "sds_motion_types.h"
#include "sds_error.h"

/* IMU CONVERSIONS */
#define RAW_GYRO_TO_RADS (float) 2000.0f*2.0f/0xFFFFf * 3.14f/180.0f
#define RAW_ACCEL_TO_GS (float) 2.0f * 2.0f/0xFFFFf

sds_return_t sds_motion_init(motion_init_t * p_motion_init);
sds_return_t sds_motion_set_state(motion_state_e motion_state);
motion_state_e sds_motion_get_state(void);
sds_return_t sds_motion_set_rate(motion_rate_e motion_rate);
motion_rate_e sds_motion_get_rate(void);
//void sds_motion_set_mode(motion_mode_e motion_mode);
motion_mode_e sds_motion_get_mode(void);
sds_return_t sds_motion_set_compass_rate(motion_rate_e compass_rate);
motion_rate_e sds_motion_get_compass_rate(void);
sds_return_t sds_motion_run_calibration(void);
void get_session_info(session_info_t * p_session_info);

#endif
