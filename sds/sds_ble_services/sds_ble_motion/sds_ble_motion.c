
#include "sdk_common.h"
#include "sds_ble_motion.h"
#include <string.h>
#include "ble_srv_common.h"
#include "sdk_config.h"
#include "sds_ble_uuid.h"

#include "sds_log_config.h"
#define NRF_LOG_MODULE_NAME b_mot
#if SDS_BLE_MOT_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_BLE_MOT_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_BLE_MOT_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_BLE_MOT_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

uint8_t packet_num = 0;

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_motion       Quaternion Orientation Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_motion_t * p_motion, ble_evt_t const * p_ble_evt)
{
    p_motion->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_motion       Quaternion Orientation Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_motion_t * p_motion, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_motion->conn_handle = BLE_CONN_HANDLE_INVALID;
}


/**@brief Function for handling write events to the data characteristic.
 *
 * @param[in]   p_motion         	Motion Service structure.
 * @param[in]   p_evt_write   		Write event received from the BLE stack.
 */
static void on_motionm_cccd_write(ble_motion_t * p_motion, ble_gatts_evt_write_t const * p_evt_write)
{
    if (p_evt_write->len == 2)
    {
        // CCCD written, update notification state
        if (false) //if (p_motion->evt_handler != NULL)
        {
            ble_motion_evt_t evt;

            if (ble_srv_is_notification_enabled(p_evt_write->data))
            {
                evt.evt_type = BLE_MOTION_EVT_NOTIFICATION_ENABLED;
            }
            else
            {
                evt.evt_type = BLE_MOTION_EVT_NOTIFICATION_DISABLED;
            }

            //p_motion->evt_handler(p_motion, &evt);
        }
    }
}

static void encode_response(sds_response_t * p_response, ble_gatts_hvx_params_t * p_hvx_params) {
	static uint8_t response_value[20];
	static uint16_t len;

	len = MIN_RESPONSE_LEN + p_response->arg_len;
	response_value[0] = p_response -> preamble;
	response_value[1] = p_response -> opcode;
	response_value[2] = p_response -> err_code;
	response_value[3] = p_response -> arg_len;
	
	if ( p_response -> arg_len && p_response->p_args != NULL ) {
		memcpy(&response_value[4], p_response->p_args, p_response->arg_len);
	}
		
	//load gatts struct
	p_hvx_params->p_data = 	response_value;
	p_hvx_params->p_len	 = 	&len;
	
	return;
}


static void on_motion_command(ble_motion_t * p_motion, ble_gatts_evt_write_t const * p_evt_write)
{
	

	NRF_LOG_DEBUG("Motion Command Recieved");
	
	/* check for valid data length */
	if (p_evt_write->len < MIN_COMMAND_LEN) {
		sds_opcode_t opcode = (sds_opcode_t) p_evt_write->data[1];
		ble_motion_command_error_send(p_motion, opcode, SDS_SHORT_COMMAND);
		return;
	}

	/* build command structure from data packet */
	sds_command_t command = {
		.preamble 	= p_evt_write->data[0],
		.opcode 	= (sds_opcode_t) p_evt_write->data[1],
		.arg_len 	= p_evt_write->data[2],
		.p_args 	= p_evt_write->data[2] ? &p_evt_write->data[3]: NULL,
	};
	
	/* check for valid preamble and opcode */
	if (command.preamble != COMMAND_PREAMBLE) {
		ble_motion_command_error_send(p_motion, command.opcode, SDS_INVALID_PREAMBLE);
		return;
	}
	else if (command.opcode < MIN_OPCODE_VAL || command.opcode > MAX_OPCODE_VAL) {
		ble_motion_command_error_send(p_motion, command.opcode, SDS_INVALID_OPCODE);
		return;
	}
	
	
	/* send command structure for main to handle */
	p_motion->command_evt_handler(&command);
	return;
}


uint32_t ble_motion_command_response_send(ble_motion_t * p_motion, sds_opcode_t opcode, uint8_t arg_len, uint8_t * p_args) {

	sds_response_t response;

	ble_gatts_hvx_params_t hvx_params;	
	hvx_params.handle = 	p_motion->motionm_handles.value_handle;
	hvx_params.type   = 	BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset = 	0;	
	
	response.preamble 	= RESPONSE_PREAMBLE;
	response.opcode 	= (sds_opcode_t) opcode;
	response.arg_len 	= arg_len;
	response.p_args 	= p_args;
	response.err_code 	= SDS_SUCCESS;		

	NRF_LOG_INFO("Response:");
	NRF_LOG_INFO("\tCommand: 0x%x", response.opcode);
	NRF_LOG_INFO("\tErr Code: 0x%x", response.err_code);
	NRF_LOG_INFO("\tLength: %d",response.arg_len);
	
	encode_response(&response, &hvx_params);
	
	/* send command response notification */
	return sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);

}

uint32_t ble_motion_command_error_send(ble_motion_t * p_motion, sds_opcode_t opcode, sds_return_t err_code) {	
	sds_response_t response;

	ble_gatts_hvx_params_t hvx_params;	
	hvx_params.handle = 	p_motion->motionm_handles.value_handle;
	hvx_params.type   = 	BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset = 	0;	
	
	response.preamble 	= RESPONSE_PREAMBLE;
	response.opcode 	= (sds_opcode_t) opcode;
	response.arg_len 	= 0;
	response.p_args 	= NULL;
	response.err_code 	= err_code;		
	
	NRF_LOG_WARNING("Command Error Response:");
	NRF_LOG_WARNING("\tCommand: 0x%x", response.opcode);
	NRF_LOG_WARNING("\tErr Code: 0x%x", response.err_code);
	
	encode_response(&response, &hvx_params);
	
	/* send response notification with error code */
	return sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);	
}

static void on_write(ble_motion_t * p_motion, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
	
	NRF_LOG_DEBUG("Write Event.")
	
	if (p_evt_write->handle == p_motion->motionm_handles.cccd_handle)
    {
        on_motionm_cccd_write(p_motion, p_evt_write);
    }
    else if (p_evt_write->handle == p_motion->motionm_handles.value_handle)
    {
		on_motion_command(p_motion, p_evt_write);
    }	
	
}


void ble_motion_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_motion_t * p_motion = (ble_motion_t *) p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_motion, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_motion, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_motion, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


static uint32_t data_char_add(ble_motion_t * p_motion, const ble_motion_init_t * p_motion_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
    uint8_t             encoded_initial_motionm[20];
	uint32_t			err_code;
	
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    cccd_md.write_perm = p_motion_init->motion_motionm_attr_md.cccd_write_perm;
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;

    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.notify = 1;
    char_md.char_props.read   = 0;
    char_md.char_props.write  = 1;	
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    // Add Custom Char UUID
    ble_uuid128_t base_uuid = {BLE_UUID_ORIENTATION_CHAR_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_motion->uuid_type);
    VERIFY_SUCCESS(err_code);
	
    ble_uuid.type = p_motion->uuid_type;
    ble_uuid.uuid = BLE_UUID_ORIENTATION_CHAR;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm  = p_motion_init->motion_motionm_attr_md.read_perm;
    attr_md.write_perm = p_motion_init->motion_motionm_attr_md.write_perm;
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 1;
    attr_md.vlen       = 1;

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = 20;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = 20;
    attr_char_value.p_value   = encoded_initial_motionm;

    return sd_ble_gatts_characteristic_add(p_motion->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           &p_motion->motionm_handles);
}





uint32_t ble_motion_init(ble_motion_t * p_motion, const ble_motion_init_t * p_motion_init)
{

	
    if (p_motion == NULL || p_motion_init == NULL)
    {
        return NRF_ERROR_NULL;
    }	
	
	uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_motion->conn_handle                 = BLE_CONN_HANDLE_INVALID;
    p_motion->command_evt_handler         = p_motion_init->command_evt_handler;
    p_motion->is_sensor_contact_supported = p_motion_init->is_sensor_contact_supported;
    p_motion->is_sensor_contact_detected  = false;
    p_motion->max_motionm_len             = 20;

	
    // Add Custom Service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_MOTION_SERVICE_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_motion->uuid_type);
    VERIFY_SUCCESS(err_code);
	
    ble_uuid.type = p_motion->uuid_type;
    ble_uuid.uuid = BLE_UUID_MOTION_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_motion->service_handle);	
	
    APP_ERROR_CHECK(err_code);

    // Add data characteristic
    err_code = data_char_add(p_motion, p_motion_init);
    APP_ERROR_CHECK(err_code);


    return NRF_SUCCESS;
}

static uint8_t quaternion_encode(uint8_t * packet, int32_t * quat, uint8_t data_flags) {
	//byte 0: 0xaa (preamble)
	//byte 1: flags
	//byte 2: packet number
	//byte 3-18: quaternion
	//byte 19: empty
	//length = 19
	packet[0] = MOTION_SAMPLE_PREAMBLE;
	packet[1] = (data_flags & QUATERNION_DATA) | (data_flags & MEMORY_DATA);
	packet[2] = packet_num++;
	memcpy(&packet[3], quat, 16);
	return 19;
}

static uint8_t imu_encode(uint8_t * packet, int16_t * gyro, int16_t * accel, uint8_t data_flags) {
	//byte 0: 0xaa (preamble)
	//byte 1: flags
	//byte 2: packet number
	//byte 3 - 8: gyro
	//byte 9 - 14: accel
	//byte 15 - 19: empty
	//length = 15
	packet[0] = MOTION_SAMPLE_PREAMBLE;
	packet[1] = (data_flags & (IMU_DATA | MEMORY_DATA));
	packet[2] = packet_num++;
	memcpy(&packet[3], gyro, 6);
	memcpy(&packet[9], accel, 6);
	return 19;
}

static uint8_t compass_encode(uint8_t * packet, int16_t * compass, uint8_t data_flags) {
	//byte 0: 0xaa (preamble)
	//byte 1: flags
	//byte 2: packet number
	//byte 3 - 8: gyro
	//byte 9 - 19: empty
	//length = 9
	packet[0] = MOTION_SAMPLE_PREAMBLE;
	packet[1] = (data_flags & COMPASS_DATA) | (data_flags & MEMORY_DATA);
	packet[2] = packet_num++;
	memcpy(&packet[3], compass, 6);
	return 9;
}


static uint8_t timestamp_encode(uint8_t * packet, timestamp_ms_t * p_timestamp, uint8_t data_flags) {
	//byte 0: 0xaa (preamble)
	//byte 1: flags
	//byte 2: packet number
	//byte 3 - 6: timestamp
	//byte 7 - 19: empty
	//length = 7
	packet[0] = MOTION_SAMPLE_PREAMBLE;
	packet[1] = (data_flags & (TIMESTAMP_DATA | MEMORY_DATA));
	packet[2] = packet_num++;
	memcpy(&packet[3], p_timestamp, 4);
	return 7;
}


uint32_t ble_motion_data_send(ble_motion_t * p_motion, motion_sample_t * p_motion_sample)
{
    uint32_t err_code = NRF_SUCCESS;

    // Send value if connected and notifying
    if (p_motion->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ble_gatts_hvx_params_t hvx_params;	
		uint8_t                encoded_data[20];   
        uint16_t               hvx_len;
		
        memset(&hvx_params, 0, sizeof(hvx_params));
		memset(encoded_data, 0, 20);
		
        hvx_params.handle = p_motion->motionm_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;

		// send notif with imu data
		if (p_motion_sample->data_flags & TIMESTAMP_DATA && p_motion_sample->data_flags & MEMORY_DATA) {	
			hvx_len = timestamp_encode(encoded_data, &p_motion_sample->timestamp, p_motion_sample->data_flags);
			hvx_params.p_len  = &hvx_len;
			hvx_params.p_data = encoded_data;
			err_code = sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);
			if (err_code) {				
                return err_code;
			}
			NRF_LOG_DEBUG("Timestamp notification Send");
		}
		
		// send notif with imu data
		if (p_motion_sample->data_flags & IMU_DATA) {	
			hvx_len = imu_encode(encoded_data, p_motion_sample->accel, p_motion_sample->gyro, p_motion_sample->data_flags);
			hvx_params.p_len  = &hvx_len;
			hvx_params.p_data = encoded_data;
			err_code = sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);
			if (err_code) {				
                return err_code;
			}
			NRF_LOG_DEBUG("IMU notification Send");
		}

		// send notif with compass data		
		if (p_motion_sample->data_flags & COMPASS_DATA) {
			hvx_len = compass_encode(encoded_data, p_motion_sample->compass, p_motion_sample->data_flags);
			hvx_params.p_len  = &hvx_len;
			hvx_params.p_data = encoded_data;
			err_code = sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);
			if (err_code) {
				return err_code;
			}
			NRF_LOG_DEBUG("Compass notification Send");
		}

		// send notif with quat data				
		if (p_motion_sample->data_flags & QUATERNION_DATA) {
			hvx_len = quaternion_encode(encoded_data, p_motion_sample->quat, p_motion_sample->data_flags);
			hvx_params.p_len  = &hvx_len;
			hvx_params.p_data = encoded_data;
			err_code = sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);			
			if (err_code) {
				return err_code;
			}
			NRF_LOG_DEBUG("Quaternion notification Send");
		}

    }
    else
    {
		NRF_LOG_ERROR("NRF_ERROR_INVALID_STATE");
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}


static uint8_t session_info_encode(uint8_t * packet, session_info_t * p_session_info) {
	//byte 0: 0xaa (preamble)
	//byte 1: flags
	//byte 2: packet number
	//byte 3 - 7: session_info
	//byte 8 - 19: empty
	//length = 8
	packet[0] = MOTION_SAMPLE_PREAMBLE;
	packet[1] = SESSION_INFO_DATA;
	packet[2] = packet_num++;
	memcpy(&packet[3], p_session_info, sizeof(session_info_t));
	return 8;
}

uint32_t ble_motion_session_info_send(ble_motion_t * p_motion, session_info_t * p_session_info) {	
	/* setup notification structure */
	ble_gatts_hvx_params_t hvx_params;	
	hvx_params.handle = 	p_motion->motionm_handles.value_handle;
	hvx_params.type   = 	BLE_GATT_HVX_NOTIFICATION;
	hvx_params.offset = 	0;	
	uint8_t                encoded_data[20];   
	uint16_t               hvx_len;
	
	NRF_LOG_DEBUG("Session Info Sent");
	
	/* encode session_info into notification structure */
	hvx_len = session_info_encode(encoded_data, p_session_info);
	hvx_params.p_len  = &hvx_len;
	hvx_params.p_data = encoded_data;	
	
	/* send session info notification */
	return sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);	
}


void ble_motion_on_gatt_evt(ble_motion_t * p_motion, nrf_ble_gatt_evt_t const * p_gatt_evt)
{
    if (    (p_motion->conn_handle == p_gatt_evt->conn_handle)
        &&  (p_gatt_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        p_motion->max_motionm_len = 20;
    }
}
