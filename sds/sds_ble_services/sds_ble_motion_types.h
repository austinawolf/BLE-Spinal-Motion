#ifndef SDS_BLE_MOTION_TYPES_H_
#define SDS_BLE_MOTION_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "sdk_common.h"
#include "sds_error.h"

#define MIN_COMMAND_LEN 3
#define MIN_RESPONSE_LEN 4

typedef uint8_t sds_preamble_t;
#define MOTION_SAMPLE_PREAMBLE      0xAA
#define COMMAND_PREAMBLE            0xBB 
#define RESPONSE_PREAMBLE           0xCC

typedef enum
{
	BASE_OFFSET = 0x20,
	GET_FW_VERSION = BASE_OFFSET,
	GET_SAMPLE_RATE,
	SET_SAMPLE_RATE,
	RUN_MOTION_CAL,
	GET_SAMPLING_STATE,
	SET_SAMPLING_STATE,
    START_SESSION,
    STOP_SESSION,
    START_MEMORY_STREAM,
    REQUEST_STREAM_DATA,
    STOP_MEMORY_STREAM,
	MIN_OPCODE_VAL = GET_FW_VERSION,
	MAX_OPCODE_VAL = STOP_MEMORY_STREAM,
} sds_opcode_t;

typedef struct {
	const uint8_t		preamble;
	const sds_opcode_t 	opcode;
	const uint8_t		arg_len;	
	const uint8_t * 	p_args;
} sds_command_t;

typedef struct {
	uint8_t				preamble;
	sds_opcode_t		opcode;
	sds_return_t		err_code;	
	uint8_t				arg_len;
	uint8_t * 			p_args;
} sds_response_t;

typedef struct {
    const uint8_t * p_data;
    uint8_t len;
} sds_notif_t;



#endif
