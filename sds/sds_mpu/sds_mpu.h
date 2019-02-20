#ifndef SDS_MPU_H_
#define SDS_MPU_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "sds_twi.h"
#include "sds_config.h"
#include "sdk_common.h"
#include "sds_motion.h"

// Sensor Config bit masks
#define SENSOR_SAMPLE_QUATERNION    (1<<0)
#define SENSOR_SAMPLE_IMU  			(1<<1)
#define SENSOR_SAMPLE_COMPASS   	(1<<2)
#define SENSOR_USE_GYRO_CAL      	(1<<3)
//#define UNUSED					(1<<4)
//#define UNUSED					(1<<5)
//#define UNUSED					(1<<6)
//#define UNUSED					(1<<7)

struct platform_data_s {
    signed char orientation[9];
};
typedef uint8_t sensor_config_t;
typedef uint32_t rate_t;

typedef struct {
	sensor_config_t sensor_config;
	rate_t mpu_rate;
	rate_t compass_rate;
} mpu_init_t;

uint32_t sds_mpu_init(mpu_init_t * p_mpu_init);
void sds_mpu_start(void);
void sds_mpu_stop(void);
void sds_mpu_set_rate(uint32_t mpu_rate);
int sds_mpu_run_self_test(void);
uint8_t sds_mpu_get_sample(motion_sample_t * motion_sample);
void sds_mpu_get_compass(motion_sample_t * motion_sample);
void sds_mpu_set_biases(int32_t * gyro, int32_t * accel);

#endif
