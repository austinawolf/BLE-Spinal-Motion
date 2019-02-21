/** @file */ 

#ifndef CONFIG_H
#define CONFIG_H

//ble config
#define DEV_MODE


//*** <<< Use Configuration Wizard in Context Menu >>> ***
//==========================================================
// <h> SDS Application Configuation

//==========================================================

// <h> Motion Configuraiton - sds_motion - Motion Library
//==========================================================

// <o> Data Collection Mode
 
// <0=> Quaternion Only
// <1=> Quaternion, IMU, Compass
// <2=> Compass Only

#ifndef SDS_MOTION_CONFIG_MODE
#define SDS_MOTION_CONFIG_MODE 1
#endif

// <o> Default Quaternion/IMU Sample Rate
 
// <0=> 1 Hz
// <1=> 2 Hz
// <2=> 5 Hz
// <3=> 10 Hz
// <4=> 20 Hz
// <5=> 40 Hz
// <6=> 60 Hz
// <7=> 80 Hz
// <8=> 100 Hz

#ifndef SDS_MOTION_DEFAULT_SAMPLE_RATE
#define SDS_MOTION_DEFAULT_SAMPLE_RATE 4
#endif

// <o> Default Compass Sample Rate
 
// <0=> 1 Hz
// <1=> 2 Hz
// <2=> 5 Hz
// <3=> 10 Hz
// <4=> 20 Hz
// <5=> 40 Hz
// <6=> 60 Hz
// <7=> 80 Hz
// <8=> 100 Hz

#ifndef SDS_COMPASS_DEFAULT_SAMPLE_RATE
#define SDS_COMPASS_DEFAULT_SAMPLE_RATE 0
#endif

// <o> Sensor Number
 
// <1=> Sensor 1
// <2=> Sensor 2
// <3=> Sensor 3
// <4=> Sensor 4


#ifndef SENSOR_NUM
#define SENSOR_NUM 4
#endif

// <o> MPU Internal Rate - Interal Sample Rate of MPU (Hz)

#ifndef MPU_INTERNAL_RATE
#define MPU_INTERNAL_RATE 100
#endif

// <q> Run Cal  - does not use saved mpu biases to allow for accurate bias calc
 
#ifndef RUN_CAL
#define RUN_CAL 0
#endif

// <o> BLE Motion Service UUID Selection - Allows for multiple devices to be used without interfering during testing

// <0=> CLINICAL
// <1=> DEBUG 1
// <2=> DEBUG 2

#ifndef SDS_BLE_MOTION_SERVICE_UUID_SELECTION
#define SDS_BLE_MOTION_SERVICE_UUID_SELECTION 0
#endif

// </h>

// <o> BLE Motion Observer Priority  
// <i> Priority with which BLE events are dispatched to the Heart Rate Service.

#ifndef BLE_MOTION_BLE_OBSERVER_PRIO
#define BLE_MOTION_BLE_OBSERVER_PRIO 2
#endif

// </h>

//*** <<< end of configuration section >>>    ***



#endif

