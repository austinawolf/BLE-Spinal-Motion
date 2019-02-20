#include "sds_mpu.h"

#include "sds_config.h"

//sds
#include "sds_motion_types.h"
#include "imu_cal.h"

//mpu9250 drivers
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "dmpKey.h"
#include "dmpmap.h"


#include "sds_log_config.h"
#define NRF_LOG_MODULE_NAME sds_mpu
#if SDS_MPU_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_MPU_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_MPU_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_MPU_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

uint32_t fifo_num = 0;
/*
static struct platform_data_s gyro_pdata = 
{
    .orientation = { 1, 0, 0,
                     0, 1, 0,
                     0, 0, 1}
};

static struct platform_data_s compass_pdata = 
{
    .orientation = {0, 1, 0,
										1, 0, 0,
										0, 0,-1}

};
*/

long gyro_bias[3] = GYRO_BIAS;
const long accel_bias[3] = ACCEL_BIAS;

uint32_t sds_mpu_init(mpu_init_t * p_mpu_init) {
	int ret;
	unsigned short gyro_rate, gyro_fsr, compass_fsr;
	unsigned char accel_fsr;
	unsigned char sensors;
	struct int_param_s int_param =
	{
		.cb = NULL,
		.pin = 0,
		.lp_exit = 0,
		.active_low = 1,
	};
		
	unsigned short dmp_features = DMP_FEATURE_6X_LP_QUAT;

	if (p_mpu_init->sensor_config & SENSOR_SAMPLE_QUATERNION) {
		sensors |= INV_XYZ_GYRO | INV_XYZ_ACCEL;
	}
	
	if (p_mpu_init->sensor_config & SENSOR_SAMPLE_IMU) {
		dmp_features |= DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO;		
	}
	
	if (p_mpu_init->sensor_config & SENSOR_USE_GYRO_CAL) {
		dmp_features |= DMP_FEATURE_GYRO_CAL;		
	}	

	if (p_mpu_init->sensor_config & SENSOR_SAMPLE_COMPASS) {
		sensors |= INV_XYZ_COMPASS;
		mpu_set_compass_sample_rate(p_mpu_init->compass_rate);
	}	
	
	//mpu init
	ret = mpu_init(&int_param);
	if (ret != 0) {
		NRF_LOG_ERROR("mpu_init ret:%d",ret);
		return ret;
	}

	//config
	ret = mpu_set_sensors(sensors);
	if (ret != 0) {
		NRF_LOG_ERROR("mpu_set_sensors ret:%d",ret);		
		return ret;		
	}	
	
	if (p_mpu_init->sensor_config & SENSOR_SAMPLE_QUATERNION) {

		mpu_set_sample_rate(MPU_INTERNAL_RATE);
		
		mpu_get_sample_rate(&gyro_rate);
		mpu_get_gyro_fsr(&gyro_fsr);
		mpu_get_accel_fsr(&accel_fsr);
		mpu_get_compass_fsr(&compass_fsr);

		NRF_LOG_DEBUG("Gyro Rate: %d",gyro_rate);
		NRF_LOG_DEBUG("Gyro FSR: %d",gyro_fsr);
		NRF_LOG_DEBUG("Accel FSR: %d",accel_fsr);
		NRF_LOG_DEBUG("Compass FSR: %d",compass_fsr);

		mpu_set_accel_bias_6500_reg(accel_bias);
		mpu_set_gyro_bias_reg(gyro_bias);

		ret = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
		if (ret != 0) {
			NRF_LOG_ERROR("mpu_configure_fifo ret:%d",ret);		
			return ret;		
		}	
		
		//dmp
		ret = dmp_load_motion_driver_firmware();
		if (ret != 0) {
			NRF_LOG_ERROR("dmp_load_motion_driver_firmware ret:%d",ret);		
			return ret;		
		}	

		dmp_register_tap_cb(NULL);
		dmp_register_android_orient_cb(NULL);

		ret = dmp_enable_feature(dmp_features);		
		if (ret != 0) {
			NRF_LOG_ERROR("dmp_enable_feature ret:%d",ret);		
			return ret;		
		}	
		
		ret = dmp_set_fifo_rate(p_mpu_init->mpu_rate);
		if (ret != 0) {
			NRF_LOG_ERROR("dmp_set_fifo_rate ret:%d",ret);		
			return ret;		
		}

		
	}
		
	NRF_LOG_INFO("MPU Init Success.",ret);
	
	return 0;		
} 

void sds_mpu_start(void) {
	int32_t ret;
	ret = mpu_set_dmp_state(1);
	if (ret != 0) {
		NRF_LOG_ERROR("mpu_set_dmp_state ret:%d",ret);		
	}
	return;	
}

void sds_mpu_stop(void) {
	int32_t ret;
	ret = mpu_set_dmp_state(0);
	if (ret != 0) {
		NRF_LOG_ERROR("mpu_set_dmp_state ret:%d",ret);		
	}
	return;
}

void sds_mpu_set_rate(rate_t mpu_rate) {
	int32_t ret;
	ret = dmp_set_fifo_rate(mpu_rate);
	if (ret != 0) {
		NRF_LOG_ERROR("dmp_set_fifo_rate ret:%d",ret);			
	}
	return;
}

uint8_t sds_mpu_get_sample(motion_sample_t * motion_sample) {

	short sensors;
	uint8_t more;
	//unsigned long compass_timestamp;
	//int compass_status;
	
	//Get quaternion and IMU data from fifo
	//Check status
	//Get number of remaining samples in fifo
	motion_sample->event = fifo_num++;
	
	motion_sample->status = dmp_read_fifo(motion_sample->gyro, motion_sample->accel, (long *) motion_sample->quat, (unsigned long *) &motion_sample->timestamp, &sensors, &more);
	if (motion_sample->status) {
		NRF_LOG_ERROR("dmp_read_fifo ret: %d",motion_sample->status);
		return 0;
	}
	else {
		NRF_LOG_INFO("Motion Sample @ %d ms",motion_sample->timestamp);			
	}
	if (sensors & INV_WXYZ_QUAT) {
		motion_sample->data_flags |= QUATERNION_DATA;
	}
	if (sensors & INV_XYZ_GYRO && sensors & INV_XYZ_ACCEL) {
		motion_sample->data_flags |= IMU_DATA;
	}	
		
	return more;	
}

void sds_mpu_get_compass(motion_sample_t * motion_sample) {
	motion_sample->event = fifo_num++;
	motion_sample->status = mpu_get_compass_reg(motion_sample->compass, &motion_sample->compass_timestamp);
	if (motion_sample->compass_status) {
		NRF_LOG_DEBUG("mpu_get_compass_reg ret: ",motion_sample->compass_status);		
	}		
	NRF_LOG_DEBUG("Compass Sample @ %d ms",motion_sample->compass_status);
	motion_sample->data_flags |= COMPASS_DATA;	
}


int sds_mpu_run_self_test(void) {
    int result;
    long gyro[3] = {0,0,0};
	long accel[3] = {0,0,0};

	mpu_set_gyro_bias_reg(gyro);
	mpu_set_accel_bias_6500_reg(accel);
	
    result = mpu_run_self_test(gyro, accel);
    if (result == 0x7) {

		/* Test passed. We can trust the gyro data here, so now we need to update calibrated data*/

        /*
         * This portion of the code uses the HW offset registers that are in the MPUxxxx devices
         * instead of pushing the cal data to the MPL software library
         */
        unsigned char i = 0;

        for(i = 0; i<3; i++) {
        	gyro[i] = (long)(gyro[i] * 32.8f); //convert to +-1000dps
        	accel[i] *=  2048.f; //convert to +-16G (bug fix from +-8G)
        	accel[i] = accel[i] >> 16;
        	gyro[i] = (long)(gyro[i] >> 16);
        }

        //mpu_set_gyro_bias_reg(gyro);
        mpu_set_accel_bias_6500_reg(accel);
		
		NRF_LOG_INFO("Passed!\n");
        NRF_LOG_INFO("accel: %ld %ld %ld\n",accel[0], accel[1],accel[2]);
        NRF_LOG_INFO("gyro: %ld %ld %ld\n",gyro[0],gyro[1],gyro[2]);
        		
		return 0;
	}	
    else {
		if (!(result & 0x1))
			NRF_LOG_INFO("Gyro failed.\n");
		if (!(result & 0x2))
			NRF_LOG_INFO("Accel failed.\n");
		if (!(result & 0x4))
			NRF_LOG_INFO("Compass failed.\n");
		return -1;
	}
}

void sds_mpu_set_biases(int32_t * gyro, int32_t * accel) {
	mpu_set_gyro_bias_reg( (long *) gyro);
	mpu_set_accel_bias_6500_reg( (long *) accel);
}	
