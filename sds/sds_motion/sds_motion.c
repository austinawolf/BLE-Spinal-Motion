#include "sds_motion.h"
#include "sds_mpu.h"
#include "app_timer.h"
#include "imu_cal.h"

#include "sds_log_config.h"
#define NRF_LOG_MODULE_NAME mot
#if SDS_MOTION_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_MOTION_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_MOTION_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_MOTION_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

typedef struct {
	void (*event_callback) (void * p_context);
	motion_rate_e motion_rate;
	motion_rate_e compass_rate;
	motion_state_e motion_state;
	motion_mode_e motion_mode;
	volatile bool compass_ready;
} motion_control_block_t;

motion_control_block_t cb;

static int motion_start(void);
static int motion_stop(void);
static void motion_timers_init(void);
static void motion_timers_start(void);
static void motion_timers_stop(void);
static void motion_event_handler(void * p_context);
static void compass_event_handler(void * p_context);
static void calibration_event_handler(void * p_context);

APP_TIMER_DEF(m_motion_timer_id);                         /**< Motion timer. */
APP_TIMER_DEF(m_compass_timer_id);                        /**< Compass timer. */
APP_TIMER_DEF(m_calibration_timer_id);                    /**< Calibration timer. */

const uint16_t sample_rate_lookup[MAX_SAMPLE_RATE - MIN_SAMPLE_RATE + 1] = 
{
	(1), 	// _1_HZ
	(2), 	// _2_HZ
	(5), 	// _5_HZ  
	(10), 	// _10_HZ
	(20), 	// _20_HZ
	(40), 	// _20_HZ
	(60), 	// _20_HZ
	(80), 	// _20_HZ
	(100),	// _100_HZ
};

/**
    @brief Lookup for getting data size of each motion mode
 */    
const uint8_t data_size_lookup[3] = 
{
	(16), 	// QUATERNION_ONLY
	(40), 	// COLLECT_ALL
	(12), 	// COMPASS_ONLY  
};

sds_return_t sds_motion_init(motion_init_t * p_motion_init) {
	
	//init mpu init structure
	mpu_init_t mpu_init = {
		.sensor_config = 0,
		.mpu_rate = sample_rate_lookup[p_motion_init->motion_rate],
		.compass_rate = 1,
	};
	
	//build motion state structure
	cb.event_callback = p_motion_init->event_callback;
	cb.motion_rate = p_motion_init->motion_rate;
	cb.compass_rate = p_motion_init->compass_rate;
	cb.motion_state = SAMPLE_OFF;
	cb.compass_ready = false;
	cb.motion_mode = p_motion_init->motion_mode;
	
	//setup mode specific config
	if (cb.motion_mode == QUATERNION_ONLY || cb.motion_mode == COLLECT_ALL) {
		mpu_init.sensor_config |= SENSOR_SAMPLE_QUATERNION;
		mpu_init.sensor_config |= SENSOR_USE_GYRO_CAL;
	}
		
	if (cb.motion_mode == COLLECT_ALL) {
		mpu_init.sensor_config |= SENSOR_SAMPLE_IMU;			
	}

	if (cb.motion_mode == COMPASS_ONLY || cb.motion_mode == COLLECT_ALL) {
		mpu_init.sensor_config |= SENSOR_SAMPLE_COMPASS;			
	}
	
	sds_mpu_init(&mpu_init);
	
	motion_timers_init();
	
	return SDS_SUCCESS;
}

motion_state_e sds_motion_get_state(void) {
	return cb.motion_state;	
}

sds_return_t sds_motion_set_state(motion_state_e motion_state) {
	if (motion_state != cb.motion_state) {
		
		switch (motion_state) {	
			case(SAMPLE_ON):
				cb.motion_state = SAMPLE_ON;
				motion_start();
				break;
			case(SAMPLE_OFF):
				cb.motion_state = SAMPLE_OFF;			
				motion_stop();
				break;
		}
		
	}
	return SDS_SUCCESS;
}

void get_session_info(session_info_t * p_session_info) {
	p_session_info->data_size = data_size_lookup[cb.motion_mode];
	p_session_info->motion_mode = cb.motion_mode;
	p_session_info->motion_rate = cb.motion_rate;
	p_session_info->compass_rate = cb.compass_rate;
}

motion_rate_e sds_motion_get_rate(void) {
	return cb.motion_rate;
}

sds_return_t sds_motion_set_rate(motion_rate_e motion_rate) {

	if ( motion_rate > MAX_SAMPLE_RATE ) {
		return SDS_INVALID_ARG;
	}	
	
    if ( cb.motion_state == SAMPLE_ON ) {
        //turn off sampling
        motion_stop();        
    }

	
	//update sample rate of timers
	cb.motion_rate = motion_rate;
	
	//update sample rate for dmp
	sds_mpu_set_rate(sample_rate_lookup[motion_rate]);
	
	NRF_LOG_INFO("Sample Rate Set to %d Hz.", sample_rate_lookup[motion_rate]);

	
    if ( cb.motion_state == SAMPLE_ON ) {
        //restore previous sample rate
        motion_start();      
    }

	return SDS_SUCCESS;
}

static int motion_start(void) {
	int ret = 0;
	
	motion_timers_start();
	if (cb.motion_mode == QUATERNION_ONLY || cb.motion_mode == COLLECT_ALL) {
		sds_mpu_start();
	}
	
	return ret;

}

motion_mode_e sds_motion_get_mode(void){
	return cb.motion_mode;
}

sds_return_t sds_motion_set_compass_rate(motion_rate_e compass_rate) {
	//turn off sampling
	motion_stop();
	
	//update sample rate of timers
	cb.compass_rate = compass_rate;
	
	NRF_LOG_INFO("Compass Rate Set to %d Hz.", sample_rate_lookup[compass_rate]);
	
	//restore previous sample rate
	motion_start();
	
	return SDS_SUCCESS;
}

motion_rate_e sds_motion_get_compass_rate(void) {
	return cb.compass_rate;
}

static int motion_stop(void) {
	int ret = 0;
	
	motion_timers_stop();
	if (cb.motion_mode == QUATERNION_ONLY || cb.motion_mode == COLLECT_ALL) {
		sds_mpu_stop();
	}	
		
	return ret;
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void motion_timers_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_create(&m_motion_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                motion_event_handler);
    APP_ERROR_CHECK(err_code);
	
    err_code = app_timer_create(&m_compass_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                compass_event_handler);
    APP_ERROR_CHECK(err_code);	

    err_code = app_timer_create(&m_calibration_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                calibration_event_handler);
    APP_ERROR_CHECK(err_code);

}


/**@brief Function for starting application timers.
 */
static void motion_timers_start(void)
{
	NRF_LOG_INFO("Motion Timers Start");
    ret_code_t err_code;

    err_code = app_timer_start(m_motion_timer_id, APP_TIMER_TICKS(1000/sample_rate_lookup[cb.motion_rate]), NULL);
    APP_ERROR_CHECK(err_code);

	if (cb.motion_mode == COMPASS_ONLY || cb.motion_mode == COLLECT_ALL) {
		err_code = app_timer_start(m_compass_timer_id, APP_TIMER_TICKS(1000/sample_rate_lookup[cb.compass_rate]), NULL);
		APP_ERROR_CHECK(err_code);
	}
}

/**@brief Function for starting application timers.
 */
static void motion_timers_stop(void)
{
	NRF_LOG_INFO("Motion Timers Stop");

    ret_code_t err_code;

    err_code = app_timer_stop(m_motion_timer_id);
    APP_ERROR_CHECK(err_code);
	
    err_code = app_timer_stop(m_compass_timer_id);
    APP_ERROR_CHECK(err_code);	
}

static void motion_event_handler(void * p_context)
{
	static motion_sample_t motion_sample;

	NRF_LOG_DEBUG("Motion Timeout Handler");

	uint8_t samples_ready;
	do {
		if (cb.motion_mode == QUATERNION_ONLY || cb.motion_mode == COLLECT_ALL) {
			memset(&motion_sample,0,sizeof(motion_sample_t));			
			samples_ready = sds_mpu_get_sample(&motion_sample);
		}
		
		if (cb.compass_ready) {
			sds_mpu_get_compass(&motion_sample);
			cb.compass_ready = false;
		}
			
		cb.event_callback(&motion_sample);
	} while (samples_ready);	
	
}

static void compass_event_handler(void * p_context)
{
	NRF_LOG_DEBUG("Compass Timeout Handler");
	cb.compass_ready = true;
}

static void calibration_event_handler(void * p_context) {
		
	imu_cal_s * p_imu_cal;
	p_imu_cal = p_context;
	
	static motion_sample_t motion_sample;

	NRF_LOG_DEBUG("Calibration Timeout Handler");

	uint8_t samples_ready;
	do {
		memset(&motion_sample,0,sizeof(motion_sample_t));			
		samples_ready = sds_mpu_get_sample(&motion_sample);

		p_imu_cal->accel_sum[X] += motion_sample.accel[X];
		p_imu_cal->accel_sum[Y] += motion_sample.accel[Y];
		p_imu_cal->accel_sum[Z] += motion_sample.accel[Z];
		p_imu_cal->gyro_sum[X] += motion_sample.gyro[X];
		p_imu_cal->gyro_sum[Y] += motion_sample.gyro[Y];
		p_imu_cal->gyro_sum[Z] += motion_sample.gyro[Z];		
		p_imu_cal->sample_num+=1;		

	} while (samples_ready);
}


sds_return_t sds_motion_run_calibration(void) {
	
	ret_code_t err_code;

	NRF_LOG_INFO("Motion Cal Initialized.");	
	
	//stop motion
	motion_stop();

	//initialize cal variables
	int32_t accel_av[XYZ] = {0, 0, 0};
	int32_t	gyro_av[XYZ] = {0, 0, 0};
	imu_cal_s imu_cal = {
		.num_of_samples = 100,
		.sample_num = 0,
		.accel_sum = {0, 0, 0}, 
		.gyro_sum = {0, 0, 0},
	};	
	
	//set sample rate
    sds_mpu_set_rate(sample_rate_lookup[_100_HZ]);
	sds_mpu_start();

	//start timer
    err_code = app_timer_start(m_calibration_timer_id, APP_TIMER_TICKS(1000/sample_rate_lookup[_100_HZ]), (void *) &imu_cal);
	APP_ERROR_CHECK(err_code);
	
	//wait for cal to finish
	NRF_LOG_INFO("Motion Cal Running...");		
	while(imu_cal.sample_num < imu_cal.num_of_samples) {
		
	}
	
	//stop sampling
    err_code = app_timer_stop(m_calibration_timer_id);
    APP_ERROR_CHECK(err_code);
	
	//calc data
	accel_av[X] = imu_cal.accel_sum[X] / imu_cal.num_of_samples;
	accel_av[Y] = imu_cal.accel_sum[Y] / imu_cal.num_of_samples;
	accel_av[Z] = imu_cal.accel_sum[Z] / imu_cal.num_of_samples;
	gyro_av[X] 	= imu_cal.gyro_sum[X] / imu_cal.num_of_samples;
	gyro_av[Y] 	= imu_cal.gyro_sum[Y] / imu_cal.num_of_samples;
	gyro_av[Z] 	= imu_cal.gyro_sum[Z] / imu_cal.num_of_samples;
	NRF_LOG_INFO("Done.");			
	NRF_LOG_INFO("Accel Offsets: x=%d, y=%d, z=%d", accel_av[X], accel_av[Y], accel_av[Z]-RAW_1G_REFERENCE);
	NRF_LOG_INFO("Gyro Offsets: x=%d, y=%d, z=%d", gyro_av[X], gyro_av[Y], gyro_av[Z]);
	
	accel_av[X] *= SCALE_ACC_OFFSET;
	accel_av[Y] *= SCALE_ACC_OFFSET;
	accel_av[Z] *= SCALE_ACC_OFFSET;
	gyro_av[X] 	*= SCALE_GYRO_OFFSET;
	gyro_av[Y] 	*= SCALE_GYRO_OFFSET;
	gyro_av[Z] 	*= SCALE_GYRO_OFFSET;	
	
	sds_mpu_set_biases(accel_av, gyro_av);
	
	//stop motion to enable viewing data
	sds_mpu_set_rate(sample_rate_lookup[cb.motion_rate]);
	sds_motion_set_state(SAMPLE_OFF);

	return SDS_SUCCESS;
}


