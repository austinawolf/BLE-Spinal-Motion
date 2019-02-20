#ifndef IMU_CAL_H_
#define IMU_CAL_H_

#include <stddef.h>
#include <stdio.h>
#include "sds_config.h"

/* IMU CONVERSIONS */
#define RAW_GYRO_TO_RADS (float) 2000.0f*2.0f/0xFFFFf * 3.14f/180.0f
#define RAW_ACCEL_TO_GS (float) 2.0f * 2.0f/0xFFFFf
	
//program biases
#define SCALE_ACC_OFFSET 1/8
#define SCALE_GYRO_OFFSET 2
#define RAW_1G_REFERENCE (uint16_t) 16383

/* IMU CAL */
#if RUN_CAL == 1
	#warning RUN_CAL enabled
	
	//mpu calibration data	
	#define GYRO_BIAS 	{	  0		*	SCALE_GYRO_OFFSET, \
							  0		*	SCALE_GYRO_OFFSET, \
							  0		*	SCALE_GYRO_OFFSET}
	#define ACCEL_BIAS 	{	  0		*	SCALE_ACC_OFFSET, \
							  0		*	SCALE_ACC_OFFSET, \
							  0		*	SCALE_ACC_OFFSET}	
	#define MAG_SOFTIRON_MATRIX		{{ 1, 0, 0 },	\
									{  0, 1, 0 },	\
									{  0, 0, 1 }}
	#define MAG_OFFSETS { 0, 0, 0 }
	#define MAG_FIELD_STRENTH 1f
	
#elif SENSOR_NUM == 1
	
	//mpu calibration data
	#define GYRO_BIAS 	{	  -24		*	SCALE_GYRO_OFFSET, \
							    0		*	SCALE_GYRO_OFFSET, \
							    1		*	SCALE_GYRO_OFFSET}
	#define ACCEL_BIAS 	{	 -109		*	SCALE_ACC_OFFSET, \
							  485		*	SCALE_ACC_OFFSET, \
							-1799		*	SCALE_ACC_OFFSET}	
	#define MAG_SOFTIRON_MATRIX		{ { 0.934, 0.005, 0.013 },	\
									{ 0.005, 0.948, 0.012 },	\
									{ 0.013, 0.012, 1.129 }}
	#define MAG_OFFSETS { -2.20F, -5.53F, -26.34F }
	#define MAG_FIELD_STRENTH 48.41f
	
#elif SENSOR_NUM == 2

	//mpu calibration data	
	#define GYRO_BIAS 	{	  -24		*	SCALE_GYRO_OFFSET, \
							    9		*	SCALE_GYRO_OFFSET, \
							  -17		*	SCALE_GYRO_OFFSET}
	#define ACCEL_BIAS 	{	  257		*	SCALE_ACC_OFFSET, \
							  213		*	SCALE_ACC_OFFSET, \
							-1631		*	SCALE_ACC_OFFSET}	
	#define MAG_SOFTIRON_MATRIX		{ { 0.934, 0.005, 0.013 },	\
									{ 0.005, 0.948, 0.012 },	\
									{ 0.013, 0.012, 1.129 }}
	#define MAG_OFFSETS { -2.20F, -5.53F, -26.34F }
	#define MAG_FIELD_STRENTH 48.41f

#elif SENSOR_NUM == 3

	//mpu calibration data	
	#define GYRO_BIAS 	{	  -17		*	SCALE_GYRO_OFFSET, \
							   16		*	SCALE_GYRO_OFFSET, \
							    6 		*	SCALE_GYRO_OFFSET}
	#define ACCEL_BIAS 	{	 1977		*	SCALE_ACC_OFFSET, \
							  782		*	SCALE_ACC_OFFSET, \
							 -881		*	SCALE_ACC_OFFSET}	
	#define MAG_SOFTIRON_MATRIX		{ { 0.934, 0.005, 0.013 },	\
									{ 0.005, 0.948, 0.012 },	\
									{ 0.013, 0.012, 1.129 }}
	#define MAG_OFFSETS { -2.20F, -5.53F, -26.34F }
	#define MAG_FIELD_STRENTH 48.41f	

#elif SENSOR_NUM == 4

	//mpu calibration data	
	#define GYRO_BIAS 	{	  -17		*	SCALE_GYRO_OFFSET, \
							   15		*	SCALE_GYRO_OFFSET, \
							    7 		*	SCALE_GYRO_OFFSET}
	#define ACCEL_BIAS 	{	 2203		*	SCALE_ACC_OFFSET, \
							 1099		*	SCALE_ACC_OFFSET, \
							 -740		*	SCALE_ACC_OFFSET}	
	#define MAG_SOFTIRON_MATRIX		{ { 0.934, 0.005, 0.013 },	\
									{ 0.005, 0.948, 0.012 },	\
									{ 0.013, 0.012, 1.129 }}
	#define MAG_OFFSETS { -2.20F, -5.53F, -26.34F }
	#define MAG_FIELD_STRENTH 48.41f	
	
#else
	#error "SENSOR NUMBER NOT DEFINED"	
#endif



#endif
