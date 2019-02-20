#ifndef SDS_MOTION_TYPES_H_
#define SDS_MOTION_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "sdk_common.h"
#include "sds_motion_types.h"

#define X 0
#define Y 1
#define Z 2
#define XYZ 3

#define ON 1
#define OFF 0

typedef uint8_t motion_packet_t;
#define QUATERNION_DATA 	(1<<0)
#define IMU_DATA 			(1<<1)
#define COMPASS_DATA 		(1<<2)
#define TIMESTAMP_DATA 		(1<<3)
#define SESSION_INFO_DATA	(1<<4)
#define MEMORY_DATA         (1<<5)

typedef uint8_t motion_rate_e;
#define _1_HZ 			0
#define _2_HZ 			1
#define _5_HZ 			2
#define _10_HZ 			3
#define _20_HZ 			4
#define _40_HZ 			5
#define _60_HZ 			6
#define _80_HZ 			7
#define _100_HZ 		8
#define MIN_SAMPLE_RATE _1_HZ
#define MAX_SAMPLE_RATE _100_HZ

typedef uint8_t motion_mode_e;
#define QUATERNION_ONLY		0
#define COLLECT_ALL      	1
#define	COMPASS_ONLY 		2
#define MIN_MOTION_MODE QUATERNION_ONLY
#define MAX_MOTION_MODE COMPASS_ONLY

typedef uint8_t motion_state_e;
#define SAMPLE_OFF 			0
#define SAMPLE_ON 			1
#define MIN_SAMPLE_STATE SAMPLE_OFF	
#define MAX_SAMPLE_STATE SAMPLE_ON

typedef unsigned long timestamp_ms_t;

typedef struct {
	void (*event_callback) (void * p_context);		
	motion_rate_e motion_rate;
	motion_rate_e compass_rate;	
	motion_mode_e motion_mode;	
} motion_init_t;

typedef struct {
    uint8_t data_size;
	motion_mode_e motion_mode;
	motion_rate_e motion_rate;
	motion_rate_e compass_rate;
} session_info_t;

typedef struct {
	uint32_t event;	
	timestamp_ms_t timestamp;
	timestamp_ms_t compass_timestamp;	
	int16_t gyro[XYZ], accel[XYZ], compass[XYZ];
	int32_t quat[4];
	uint8_t data_flags;	
	int8_t status;
	int8_t compass_status;
} motion_sample_t;

typedef struct {
	uint16_t num_of_samples;
	volatile uint32_t sample_num;
	int32_t accel_sum[XYZ];
	int32_t gyro_sum[XYZ];
} imu_cal_s;

#endif
