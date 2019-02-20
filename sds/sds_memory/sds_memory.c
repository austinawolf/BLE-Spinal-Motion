/** @file */ 

#include "sds_memory.h"
#include "nrf_drv_qspi.h"
#include "nrf_delay.h"

//sds
#include "sds_clock.h"

#ifdef USE_SDS_LOG_CONFIG
#include "sds_log_config.h"
#define NRF_LOG_MODULE_NAME mem
#if SDS_MEM_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_MEM_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_MEM_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_MEM_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#endif
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define QSPI_STD_CMD_WRSR   0x01
#define QSPI_STD_CMD_RSTEN  0x66
#define QSPI_STD_CMD_RST    0x99

#define QSPI_TEST_BLOCK_SIZE 4096

#define SESSION_INFO_BLOCK_INDEX 1
#define MIN_BLOCK_INDEX 2
#define MAX_BLOCK_INDEX 16384

#define WAIT_FOR_PERIPH() do { \
        while (!cb.m_finished) {} \
        cb.m_finished = false;    \
    } while (0)

#define READ_FROM_BUFFER(_DEST) do { \
        memcpy(&_DEST, m_buffer_rx + cb.block_address, sizeof(_DEST)); \
        cb.block_address += sizeof(_DEST); \
    } while {0}
	
#define WRITE_TO_BUFFER(_SOURCE) do { \
        memcpy(m_buffer_tx + cb.block_address, &SOURCE, sizeof(_SOURCE)); \
        cb.block_address += sizeof(_SOURCE); \
    } while {0}

/**
    @brief Write buffer
 */    
static uint8_t m_buffer_tx[QSPI_TEST_BLOCK_SIZE];

/**
    @brief Read buffer
 */    
static uint8_t m_buffer_rx[QSPI_TEST_BLOCK_SIZE];

/**
    @brief hold memory control info
 */       
static memory_control_block_t cb;

static void configure_memory(void);
static void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context);
static void erase_block(void);
static uint32_t erase_all(void);
static uint32_t write_buffer_to_block(void);
static uint32_t read_block_to_buffer(void);
static uint32_t verify_write(void);    
static void inc_block_index(void);
static void reset_block_index(void);
static void print_read_buffer(void);
static void timestamp_block(timestamp_ms_t timestamp);
static void initialize_block(void);
static void read_session_info(session_info_t * p_session_info);
static void write_session_info(session_info_t * p_session_info);
static void stream_motion_block(void);
static sds_return_t get_next_sample(motion_sample_t * p_motion_sample);


void sds_mem_init(memory_init_s * p_memory_init) {
    
    uint32_t err_code;

    nrf_drv_qspi_config_t config = NRF_DRV_QSPI_DEFAULT_CONFIG;
    
    cb.event_callback = p_memory_init->event_callback;
    cb.memory_mode = SDS_MEMORY_MODE_IDLE;
    cb.m_finished = false;
    
    err_code = nrf_drv_qspi_init(&config, qspi_handler, NULL);
    APP_ERROR_CHECK(err_code);

    configure_memory();

    NRF_LOG_INFO("SDS Memory Initialized");
    
}

sds_return_t sds_mem_start_session(session_info_t * p_session_info) {

	if (cb.memory_mode != SDS_MEMORY_MODE_IDLE) {
		return SDS_INVALID_STATE;
	}
	    
	/* Setup control block */
    cb.motion_mode = p_session_info->motion_mode;
    cb.data_size = p_session_info->motion_mode;
    cb.use_next_timestamp = false;
    cb.block_index = MIN_BLOCK_INDEX;
    cb.memory_mode = SDS_MEMORY_MODE_SESSION;
	
	/* Write session info to session info block */
    write_session_info(p_session_info);
    
	/* Reset block index and setup first block for writing */
    reset_block_index();
    initialize_block();
        
    cb.event_callback(SDS_MEMORY_EVT_SESSION_STARTED, NULL);    
    return SDS_SUCCESS;
}

sds_return_t sds_mem_stop_session(void) {

	if (cb.memory_mode != SDS_MEMORY_MODE_SESSION) {
		return SDS_INVALID_STATE;
	}
    
    cb.memory_mode = SDS_MEMORY_MODE_IDLE;
    
    cb.event_callback(SDS_MEMORY_EVT_SESSION_TERMINATED, NULL);
    
    return SDS_SUCCESS;
}



sds_return_t sds_mem_save_motion_data(motion_sample_t * p_motion_sample) {

	if (cb.memory_mode != SDS_MEMORY_MODE_SESSION) {
		return SDS_INVALID_STATE;
	}
	
    if (cb.use_next_timestamp) {
        timestamp_ms_t timestamp = p_motion_sample->timestamp;
        timestamp_block(timestamp);
        cb.use_next_timestamp = false;
    }
    
    switch (cb.motion_mode) {
        
        case QUATERNION_ONLY:

            memcpy(m_buffer_tx + cb.block_address, p_motion_sample->quat, sizeof(p_motion_sample->quat));
            cb.block_address += sizeof(p_motion_sample->quat);
        
            break;

        case COLLECT_ALL:

            memcpy(m_buffer_tx + cb.block_address, p_motion_sample->quat, sizeof(p_motion_sample->quat));
            cb.block_address += sizeof(p_motion_sample->quat);

            memcpy(m_buffer_tx + cb.block_address, p_motion_sample->accel, sizeof(p_motion_sample->accel));
            cb.block_address += sizeof(p_motion_sample->accel);

            memcpy(m_buffer_tx + cb.block_address, p_motion_sample->gyro, sizeof(p_motion_sample->gyro));
            cb.block_address += sizeof(p_motion_sample->gyro);
                
            memcpy(m_buffer_tx + cb.block_address, p_motion_sample->compass, sizeof(p_motion_sample->compass));
            cb.block_address += sizeof(p_motion_sample->compass);   
        
            break;

        case COMPASS_ONLY:

            memcpy(m_buffer_tx + cb.block_address, p_motion_sample->compass, sizeof(p_motion_sample->compass));
            cb.block_address += sizeof(p_motion_sample->compass);
        
            break;        
    }
        
    if (cb.block_address > QSPI_TEST_BLOCK_SIZE - cb.data_size) {
        NRF_LOG_DEBUG("Buffer Full. Writing to block.");
        write_buffer_to_block();
        read_block_to_buffer();
        verify_write();
        inc_block_index();
        initialize_block();        
    }
      
    return SDS_SUCCESS;
}

sds_return_t sds_mem_start_stream(session_info_t * p_session_info) {
    
    block_preamble_t preamble;	

	if (cb.memory_mode != SDS_MEMORY_MODE_IDLE) {
		return SDS_INVALID_STATE;
	}
       
    reset_block_index();

    read_session_info(p_session_info);
  
    cb.data_size = p_session_info->data_size;
    cb.motion_mode = p_session_info->motion_mode;
    cb.use_next_timestamp = true;
    cb.memory_mode = SDS_MEMORY_MODE_STREAM;    
    
    cb.event_callback(SDS_MEMORY_EVT_STREAM_STARTED, NULL);
	
    read_block_to_buffer();
    
    //check preamble
    memcpy(&preamble, m_buffer_rx + cb.block_address, sizeof(preamble));
    cb.block_address += sizeof(preamble);
    
    if (preamble != TIMESTAMP_PREAMBLE) {
        return SDS_MEMORY_NO_DATA;
    }      
    return SDS_SUCCESS;   
}

sds_return_t sds_stream_request_samples(uint8_t number_of_samples) {

    static motion_sample_t motion_sample;
    sds_return_t err_code;
	
	if (cb.memory_mode != SDS_MEMORY_MODE_STREAM) {
		return SDS_INVALID_STATE;
	}
    	
    for (int i = 0; i < number_of_samples; i++) {
        err_code = get_next_sample(&motion_sample);
		if (err_code == SDS_INVALID_PREAMBLE) {
			break;
		}
        cb.event_callback(SDS_MEMORY_EVT_MOTION_DATA, &motion_sample);
        memset(&motion_sample, 0, sizeof(motion_sample));
    }  
    return SDS_SUCCESS;
}

sds_return_t sds_mem_stop_stream(void) {

	if (cb.memory_mode != SDS_MEMORY_MODE_STREAM) {
		return SDS_INVALID_STATE;
	}
    
    cb.memory_mode = SDS_MEMORY_MODE_IDLE;    
    
    return SDS_SUCCESS;
}

/**
    * @brief called when a read/write/erase is done
*/
static void qspi_handler(nrf_drv_qspi_evt_t event, void * p_context)
{
    UNUSED_PARAMETER(event);
    UNUSED_PARAMETER(p_context);
    cb.m_finished = true;
}

/**
    * @brief read session info from the session info block
*/
static void read_session_info(session_info_t * p_session_info) {
    
    uint32_t err_code;    

    err_code = nrf_drv_qspi_read(p_session_info, sizeof(session_info_t), SESSION_INFO_BLOCK_INDEX * QSPI_TEST_BLOCK_SIZE);
    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
      
}

/**
    * @brief writes session info to the session info block
*/
static void write_session_info(session_info_t * p_session_info) {

    uint32_t err_code;
	
	cb.m_finished = true;
    err_code = nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, SESSION_INFO_BLOCK_INDEX * QSPI_TEST_BLOCK_SIZE);
    APP_ERROR_CHECK(err_code);   
	
	uint32_t ms_time = 0;
	uint32_t timeout_ms = 100;
	while (nrf_drv_qspi_mem_busy_check() == NRF_ERROR_BUSY)
	{
		if (ms_time++ > timeout_ms) {
			APP_ERROR_CHECK(NRF_ERROR_TIMEOUT);
		}
		nrf_delay_ms(1);
	};
	
    err_code = nrf_drv_qspi_write(p_session_info, sizeof(session_info_t), SESSION_INFO_BLOCK_INDEX * QSPI_TEST_BLOCK_SIZE);
    APP_ERROR_CHECK(err_code);  
}


static sds_return_t get_next_sample(motion_sample_t * p_motion_sample) {
    
    block_preamble_t preamble;

    p_motion_sample->data_flags |= MEMORY_DATA;    
    
    if (cb.block_address > QSPI_TEST_BLOCK_SIZE - cb.data_size) {
        NRF_LOG_DEBUG("End of buffer.");
      
        //get data
        inc_block_index();
        read_block_to_buffer();

        //check preamble
        memcpy(&preamble, m_buffer_rx + cb.block_address, sizeof(preamble));
        cb.block_address += sizeof(preamble);      
        if (preamble != TIMESTAMP_PREAMBLE) {
			sds_mem_stop_stream();
            cb.event_callback(SDS_MEMORY_EVT_STREAM_TERMINATED, NULL);
			return SDS_INVALID_PREAMBLE;
        }
        
        cb.use_next_timestamp = true;
    }

    if (cb.use_next_timestamp) {
        //get timestamp
        memcpy(&p_motion_sample->timestamp, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->timestamp));
        cb.block_address += sizeof(p_motion_sample->timestamp);      
        p_motion_sample->data_flags |= TIMESTAMP_DATA;
        cb.use_next_timestamp = false;
		NRF_LOG_INFO("Streaming Timestamp: %d ms.", p_motion_sample->timestamp);
    }
    
    switch (cb.motion_mode) {
        
        case QUATERNION_ONLY:
            
            memcpy(&p_motion_sample->quat, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->quat));
            cb.block_address += sizeof(p_motion_sample->quat);      
            p_motion_sample->data_flags |= QUATERNION_DATA;
			NRF_LOG_INFO("Streaming Quaternion Sample.");
            break;
        
        case COLLECT_ALL:
                           
            memcpy(&p_motion_sample->quat, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->quat));
            cb.block_address += sizeof(p_motion_sample->quat);      
            p_motion_sample->data_flags |= QUATERNION_DATA;         
            
            memcpy(&p_motion_sample->accel, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->accel));
            cb.block_address += sizeof(p_motion_sample->accel);              
            memcpy(&p_motion_sample->gyro, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->gyro));
            cb.block_address += sizeof(p_motion_sample->gyro);      
            p_motion_sample->data_flags |= IMU_DATA;         

            memcpy(&p_motion_sample->compass, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->compass));
            cb.block_address += sizeof(p_motion_sample->compass);      
            p_motion_sample->data_flags |= COMPASS_DATA;                     
            NRF_LOG_INFO("Streaming Quat/IMU/Compass Sample.");
			break;        

        case COMPASS_ONLY:
                        
            memcpy(&p_motion_sample->compass, m_buffer_rx + cb.block_address, sizeof(p_motion_sample->compass));
            cb.block_address += sizeof(p_motion_sample->compass);      
            p_motion_sample->data_flags |= COMPASS_DATA;  
            NRF_LOG_INFO("Streaming Compass Sample.");            
			break;         
    }
    
	return SDS_SUCCESS;
}

/**
    * @brief configures qspi and memory
*/
static void configure_memory() {
    
    uint8_t temporary = 0x40;
    uint32_t err_code;
    nrf_qspi_cinstr_conf_t cinstr_cfg = {
        .opcode    = QSPI_STD_CMD_RSTEN,
        .length    = NRF_QSPI_CINSTR_LEN_1B,
        .io2_level = true,
        .io3_level = true,
        .wipwait   = true,
        .wren      = true
    };

    // Send reset enable
    err_code = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    // Send reset command
    cinstr_cfg.opcode = QSPI_STD_CMD_RST;
    err_code = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    // Switch to qspi mode
    cinstr_cfg.opcode = QSPI_STD_CMD_WRSR;
    cinstr_cfg.length = NRF_QSPI_CINSTR_LEN_2B;
    err_code = nrf_drv_qspi_cinstr_xfer(&cinstr_cfg, &temporary, NULL);
    APP_ERROR_CHECK(err_code);
        
}

/**
    * @brief adds timestamp to block
*/
static void timestamp_block(timestamp_ms_t timestamp) {
    
    memcpy(m_buffer_tx + cb.block_address, &timestamp, sizeof(timestamp));
    cb.block_address += sizeof(timestamp);
    
}

/**
    * @brief resets block address pointer and erases block, sets up block to be timestamped
*/
static void initialize_block(void) {
    
    
    const block_preamble_t preamble = TIMESTAMP_PREAMBLE;

    memset(m_buffer_tx, 0, QSPI_TEST_BLOCK_SIZE);
    
    cb.block_address = 0;    
    erase_block();
       
    memcpy(m_buffer_tx + cb.block_address, &preamble, sizeof(preamble));
    cb.block_address += sizeof(preamble);      
    
    cb.use_next_timestamp = true;

    NRF_LOG_DEBUG("Block Initialized");
    
}

/**
    * @brief erases block at block index
*/
static void erase_block(void) {

    uint32_t err_code;    

    cb.m_finished = false;
	   
    err_code = nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_4KB, cb.block_index * QSPI_TEST_BLOCK_SIZE);
    APP_ERROR_CHECK(err_code);
	
	WAIT_FOR_PERIPH();
	
    NRF_LOG_DEBUG("Block %d Erase Complete.", cb.block_index);
}

/**
    * @brief erases entire memory. takes very long
*/
static uint32_t erase_all(void) {

    uint32_t err_code;    

    cb.m_finished = false;
    
    err_code = nrf_drv_qspi_erase(NRF_QSPI_ERASE_LEN_ALL, 0);
    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
    NRF_LOG_DEBUG("Erased All.", cb.block_index);

    return err_code;

}

/**
    * @brief writes tx buffer to current block
*/
static uint32_t write_buffer_to_block(void) {

    uint32_t err_code;
    
    err_code = nrf_drv_qspi_write(m_buffer_tx, QSPI_TEST_BLOCK_SIZE, cb.block_index * QSPI_TEST_BLOCK_SIZE);
    APP_ERROR_CHECK(err_code);
    WAIT_FOR_PERIPH();
    NRF_LOG_DEBUG("Block %d Write Complete.", cb.block_index);

    return err_code;
    
}

/**
    * @brief reads current block to rx buffer
*/
static uint32_t read_block_to_buffer(void) {

    uint32_t err_code;

    memset(m_buffer_rx, 0, QSPI_TEST_BLOCK_SIZE);
    
    err_code = nrf_drv_qspi_read(m_buffer_rx, QSPI_TEST_BLOCK_SIZE, cb.block_index * QSPI_TEST_BLOCK_SIZE);
    WAIT_FOR_PERIPH();
    NRF_LOG_DEBUG("Block %d Read Complete.", cb.block_index);

    return err_code;
    
}

/**
    * @brief checks that write buffer and read buffer match
*/ 
static uint32_t verify_write(void) {
    
    uint32_t err_code;

    NRF_LOG_DEBUG("Compare...");
    if (memcmp(m_buffer_tx, m_buffer_rx, QSPI_TEST_BLOCK_SIZE) == 0)
    {
        NRF_LOG_DEBUG("Data consistent");
    }
    else
    {
        NRF_LOG_WARNING("Data inconsistent");
    }
    return err_code;
}

/**
    * @brief increments block index
*/ 
static void inc_block_index(void) {
    
    cb.block_index++;
    cb.block_address = 0;
    
    if (cb.block_index > MAX_BLOCK_INDEX) {
        
        if (cb.memory_mode == SDS_MEMORY_MODE_SESSION) {
            
            cb.event_callback(SDS_MEMORY_EVT_FLASH_FULL,NULL);
            sds_mem_stop_session();
            
        }
        else if (cb.memory_mode == SDS_MEMORY_MODE_STREAM) {

            sds_mem_stop_stream();
            
        }
        else {
            NRF_LOG_ERROR("Unexpected Result");
        }        
        
    }
    
}

/**
    * @brief moves block index back to minimum value
*/ 
static void reset_block_index(void) {
    cb.block_index = MIN_BLOCK_INDEX;
    cb.block_address = 0;    
}
