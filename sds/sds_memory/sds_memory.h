/** @file */ 

#ifndef SDS_MEMORY_H_
#define SDS_MEMORY_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"

//sds
#include "sds_motion_types.h"
#include "sds_error.h"

#define TIMESTAMP_PREAMBLE 0xaaaa

typedef enum
{
    SDS_MEMORY_EVT_SESSION_STARTED,
    SDS_MEMORY_EVT_FLASH_FULL,    
    SDS_MEMORY_EVT_SESSION_TERMINATED,    
    SDS_MEMORY_EVT_STREAM_STARTED,   
    SDS_MEMORY_EVT_MOTION_DATA,
    SDS_MEMORY_EVT_STREAM_TERMINATED,    
} sds_memory_evt_t;

typedef enum
{
    SDS_MEMORY_MODE_IDLE,
    SDS_MEMORY_MODE_SESSION,
    SDS_MEMORY_MODE_STREAM,
} sds_memory_mode;

typedef struct {
    void (*event_callback) (sds_memory_evt_t event, void * p_context);
    uint32_t block_index;
    uint32_t block_address;
    uint32_t data_size;   
    bool use_next_timestamp;
	motion_mode_e motion_mode;
    volatile sds_memory_mode memory_mode;
    volatile bool m_finished;    
} memory_control_block_t;

typedef struct {
    void (*event_callback) (sds_memory_evt_t event, void * p_context);
} memory_init_s;

typedef struct {
    uint32_t info_a[4];
} datatype;

typedef uint16_t block_preamble_t;

/**
    * @brief initializes qspi drivers, configures memory, and sets up callback
    * @param p_memory_init pointer to init structure
    * @return
*/
void sds_mem_init(memory_init_s * p_memory_init);

/**
    * @brief starts memory session and allows for motion data to be saved
    * @param p_session_info contains runtime motion configuration info
    * @return
*/
sds_return_t sds_mem_start_session(session_info_t * p_session_info);

/**
    * @brief saves motion data to the next location in memory
    * @param p_motion_sample sample to be saved
    * @return
*/
sds_return_t sds_mem_save_motion_data(motion_sample_t * p_motion_sample);

/**
    * @brief stops memory session
    * @return
*/
sds_return_t sds_mem_stop_session(void);

/**
    * @brief starts memory session, data will be available through the callback when ready
    * @return
*/
sds_return_t sds_mem_stop_session(void);

/**
    * @brief starts memory stream, data will be available through the callback on SDS_MEMORY_EVT_MOTION_DATA event
    * @return
*/
sds_return_t sds_mem_start_stream(session_info_t * p_session_info);

/**
    * @brief stops memory stream
    * @return
*/
sds_return_t sds_mem_stop_stream(void);


/**
    * @brief generates streamed samples through callback
    * @return
*/
sds_return_t sds_stream_request_samples(uint8_t number_of_samples);

 
#endif
