/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 * @defgroup qspi_example_main main.c
 * @{
 * @ingroup qspi_example
 *
 * @brief QSPI Example Application main file.
 *
 * This file contains the source code for a sample application using QSPI.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nrf_delay.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "boards.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "sdk_config.h"

//sds
#include "sds_memory.h"
#include "sds_motion.h"


void memory_event_handler(sds_memory_evt_t event, void * p_context);
    
int main(void)
{
    uint32_t err_code;

    err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("QSPI write and read example using 24bit addressing mode");
	NRF_LOG_FLUSH();
	
    memory_init_s memory_init;
    memory_init.event_callback = memory_event_handler;
    
    sds_memory_init(&memory_init);

    session_info_t session_info;
    session_info.motion_mode = COLLECT_ALL;
    session_info.motion_rate = _10_HZ;
    session_info.compass_rate = _10_HZ;
    
    start_session(&session_info);
        
    int32_t i = 0;

    motion_sample_t motion_sample;

    for (i = 0; i < 2000; i++) {

        motion_sample.accel[0] = 10;
        motion_sample.accel[1] = 11;         
        motion_sample.accel[2] = 12;         
        motion_sample.gyro[0] = 13;
        motion_sample.gyro[1] = 14;         
        motion_sample.gyro[2] = 15; 
        motion_sample.compass[0] = 16; 
        motion_sample.compass[1] = 17;         
        motion_sample.compass[2] = 18;          
        motion_sample.quat[0] = 19;       
        motion_sample.quat[1] = 20;
        motion_sample.quat[2] = 21;        
        motion_sample.quat[3] = 22;
        motion_sample.timestamp = 99;
            
        save_motion_data(&motion_sample);
    }

    stop_session();
    start_data_stream();
    
    for (;;)
    {
        
    }      
}

void memory_event_handler(sds_memory_evt_t event, void * p_context) {

    motion_sample_t motion_sample;

    
    switch (event) {
    
        case(SDS_MEMORY_EVT_SESSION_STARTED):        
            NRF_LOG_INFO("Memory session started.");
            break;
        
        case(SDS_MEMORY_EVT_FLASH_FULL):
            NRF_LOG_INFO("Flash Memory Full.");        
            break;   
        
        case(SDS_MEMORY_EVT_SESSION_TERMINATED):
            NRF_LOG_INFO("Memory session terminated.");            
            break;
        
        case(SDS_MEMORY_EVT_STREAM_STARTED):
            NRF_LOG_INFO("Memory stream started.");            
            break; 
        
        case(SDS_MEMORY_EVT_MOTION_DATA):
            
            memcpy(&motion_sample, p_context, sizeof(motion_sample));
        
            //NRF_LOG_INFO("New Motion Data.");
            //NRF_LOG_INFO("q0=%d, q1=%d, q3=%d, q4=%d.",motion_sample.quat[0], motion_sample.quat[1], motion_sample.quat[2], motion_sample.quat[3] );        
            //NRF_LOG_INFO("ax=%d, ay=%d, az=%d",motion_sample.accel[0], motion_sample.accel[1], motion_sample.accel[2]);        
            //NRF_LOG_INFO("gx=%d, gy=%d, gz=%d",motion_sample.gyro[0], motion_sample.gyro[1], motion_sample.gyro[2]);        
            //NRF_LOG_INFO("cx=%d, cy=%d, cz=%d",motion_sample.compass[0], motion_sample.compass[1], motion_sample.compass[2]);        
            break;
        
        case(SDS_MEMORY_EVT_STREAM_TERMINATED):
            NRF_LOG_INFO("Memory stream terminated.");        
            break;        
    }
    
}

/** @} */
