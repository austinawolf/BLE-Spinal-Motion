#ifndef SDS_BLE_UUID_H_
#define SDS_BLE_UUID_H_

#include <stdint.h>
#include <stdbool.h>
#include "ble_types.h"
#include "app_util.h"


#define IGNORED 0x00

/** @defgroup 
 * @{ */
#define BLE_UUID_MOTION_SERVICE_BASE BLE_UUID_MOTION_SERVICE_BASE_CLINICAL
#define BLE_UUID_MOTION_SERVICE_BASE_CLINICAL 	{0x03,0xb0,0xf8,0x4b,0x02,0xd9,0xa9,0xb9,0xca,0x40,0x2c,0xc3,IGNORED,IGNORED,0x44,0x92} //{924418ff-c32c-40ca-b9a9-d9024bf8b001}
#define BLE_UUID_MOTION_SERVICE_BASE_DEBUG1 	{0x02,0xb0,0xf8,0x4b,0x02,0xd9,0xa9,0xb9,0xca,0x40,0x2c,0xc3,IGNORED,IGNORED,0x44,0x92} //{924418ff-c32c-40ca-b9a9-d9024bf8b002}
#define BLE_UUID_MOTION_SERVICE_BASE_DEBUG2 	{0x01,0xb0,0xf8,0x4b,0x02,0xd9,0xa9,0xb9,0xca,0x40,0x2c,0xc3,IGNORED,IGNORED,0x44,0x92} //{924418ff-c32c-40ca-b9a9-d9024bf8b003}
#define BLE_UUID_MOTION_SERVICE 0x18FF

/** @defgroup
 * @{ */
#define BLE_UUID_ORIENTATION_CHAR_BASE 	{0x01,0x12,0xc9,0x8e,0x8c,0x67,0x60,0xb6,0xbb,0x41,0x64,0x82,IGNORED,IGNORED,0x19,0x43} //{43192aff-8264-41bb-b660-678c8ec91201}
#define BLE_UUID_ORIENTATION_CHAR 0x2AFF   /**< Orientation characteristic UUID. */
#define BLE_UUID_COMMAND_CHAR_BASE 		{0x01,0xad,0x59,0xa1,0x0f,0x1e,0x48,0xa5,0x17,0x4a,0xc0,0xa7,IGNORED,IGNORED,0x54,0xd9} //{d9541770-a7c0-4a17-a548-1e0fa159ad01}
#define BLE_UUID_COMMAND_CHAR 0x1770   /**< Command Orientation characteristic UUID. */	
	
#endif
