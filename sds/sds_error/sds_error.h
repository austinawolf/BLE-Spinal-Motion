/** @file */ 


#ifndef SDS_ERROR_H_
#define SDS_ERROR_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

typedef enum
{
	SDS_SUCCESS,
	SDS_ERROR,
	SDS_INVALID_ARG,
	SDS_INVALID_ARG_LEN,
	SDS_INVALID_OPCODE,
	SDS_INVALID_PREAMBLE,
	SDS_SHORT_COMMAND,
	SDS_MOTION_SELF_TEST_FAILURE,
	SDS_INVALID_STATE,
	SDS_QSPI_ERROR,
	SDS_UNHANDLED_COMMAND,
    SDS_MEMORY_NO_DATA,
} sds_return_t;

#endif

