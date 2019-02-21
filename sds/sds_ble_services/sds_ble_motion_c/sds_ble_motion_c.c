
#include "stdio.h"
#include "sdk_common.h"
#include "sds_ble_motion_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"
#include "nordic_common.h"
#include "app_error.h"

//sds
#include "sds_motion_types.h"
#include "sds_command.h"

#include "sds_log_config.h"
#define NRF_LOG_MODULE_NAME b_mot
#if SDS_BLE_MOT_C_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_BLE_MOT_C_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_BLE_MOT_C_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_BLE_MOT_C_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */
#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN    /**< Length of the write message for CCCD. */

typedef enum
{
    READ_REQ,  /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ  /**< Type identifying that this tx_message is a write request. */
} tx_request_t;


typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];  /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                       /**< GATTC parameters for this message. */
} write_params_t;

typedef struct
{
    uint16_t     conn_handle;  /**< Connection handle to be used when transmitting this message. */
    tx_request_t type;         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t       read_handle;  /**< Read request message. */
        write_params_t write_req;    /**< Write request message. */
    } req;
} tx_message_t;


static tx_message_t  m_tx_buffer[TX_BUFFER_SIZE];  /**< Transmit buffer for messages to be transmitted to the central. */
static uint32_t      m_tx_insert_index = 0;        /**< Current index in the transmit buffer where the next message should be inserted. */
static uint32_t      m_tx_index = 0;               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */




static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            m_tx_index++;
            m_tx_index &= TX_BUFFER_MASK;
        }
        else
        {
            NRF_LOG_DEBUG("SD Read/Write API returns error. This message sending will be "
                          "attempted again..");
        }
    }
}

static void on_write_rsp(ble_motion_c_t * p_ble_motion_c, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_ble_motion_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}

static void on_hvx(ble_motion_c_t * p_ble_motion_c, const ble_evt_t * p_ble_evt)
{	
	// Check if the event is on the link for this instance
    if (p_ble_motion_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        NRF_LOG_DEBUG("Received HVX on link 0x%x, not associated to instance 0x%x, ignore",
                      p_ble_evt->evt.gattc_evt.conn_handle, p_ble_motion_c->conn_handle);   
		return;
    }
	
    NRF_LOG_DEBUG("Received HVX on link 0x%x, motionm_handle 0x%x",p_ble_evt->evt.gattc_evt.params.hvx.handle,p_ble_motion_c->peer_motion_db.motionm_handle);
	
    // Check if this is a quaternion orientation notification.
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_motion_c->peer_motion_db.motionm_handle)
    {
        ble_motion_c_evt_t ble_motion_c_evt;
        ble_motion_c_evt.conn_handle = p_ble_motion_c->conn_handle;
        ble_motion_c_evt.evt_type = BLE_MOTION_C_EVT_NOTIFICATION;

        sds_notif_t notif;
        notif.p_data = p_ble_evt->evt.gattc_evt.params.hvx.data;
        notif.len  = p_ble_evt->evt.gattc_evt.params.hvx.len;
        
        p_ble_motion_c->evt_handler(p_ble_motion_c, &ble_motion_c_evt, &notif);
    }
}

static void on_disconnected(ble_motion_c_t * p_ble_motion_c, const ble_evt_t * p_ble_evt)
{
    if (p_ble_motion_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_ble_motion_c->conn_handle                 = BLE_CONN_HANDLE_INVALID;
        p_ble_motion_c->peer_motion_db.motionm_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_ble_motion_c->peer_motion_db.motionm_handle      = BLE_GATT_HANDLE_INVALID;
    }
}


void ble_motion_on_db_disc_evt(ble_motion_c_t * p_ble_motion_c, const ble_db_discovery_evt_t * p_evt)
{
	NRF_LOG_DEBUG("evt_type: %d", p_evt->evt_type);
	NRF_LOG_DEBUG("uuid: 0x%x", p_evt->params.discovered_db.srv_uuid.uuid);
	NRF_LOG_DEBUG("type: %d", p_evt->params.discovered_db.srv_uuid.type);
	
    // Check if the Quaternion Orientation Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_MOTION_SERVICE)
    {
        // Find the CCCD Handle of the Quaternion Orientation Measurement characteristic.
        uint32_t i;

        ble_motion_c_evt_t evt;

        evt.evt_type    = BLE_MOTION_C_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;

		NRF_LOG_DEBUG("Chars found: %d", p_evt->params.discovered_db.char_count);
        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
			NRF_LOG_DEBUG("Char discovery: 0x%x, %d", p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid, p_evt->params.discovered_db.charateristics[i].characteristic.uuid.type)
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_ORIENTATION_CHAR)
            {
                // Found Quaternion Orientation characteristic. Store CCCD handle and break.
                evt.params.peer_db.motionm_cccd_handle = p_evt->params.discovered_db.charateristics[i].cccd_handle;
                evt.params.peer_db.motionm_handle = p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
				NRF_LOG_DEBUG("Found Motion Char: 0x%x",evt.params.peer_db.motionm_handle);
                break;			
            }		
        }

        NRF_LOG_DEBUG("Quaternion Service discovered at peer.");
        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_ble_motion_c->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if ((p_ble_motion_c->peer_motion_db.motionm_cccd_handle == BLE_GATT_HANDLE_INVALID)&&
                (p_ble_motion_c->peer_motion_db.motionm_handle == BLE_GATT_HANDLE_INVALID))
            {
                p_ble_motion_c->peer_motion_db = evt.params.peer_db;
            }
        }


        p_ble_motion_c->evt_handler(p_ble_motion_c, &evt, NULL);
    }
}


uint32_t ble_motion_c_init(ble_motion_c_t * p_ble_motion_c, ble_motion_c_init_t * p_ble_motion_c_init)
{
	uint32_t err_code;
	
    VERIFY_PARAM_NOT_NULL(p_ble_motion_c);
    VERIFY_PARAM_NOT_NULL(p_ble_motion_c_init);

	NRF_LOG_DEBUG("BLE motion Central Services Init");
    
    // Add Custom Service UUID
    ble_uuid_t motion_uuid;	
    ble_uuid128_t base_uuid = {BLE_UUID_MOTION_SERVICE_BASE};
    err_code =  sd_ble_uuid_vs_add(&base_uuid, &motion_uuid.type);
	motion_uuid.uuid = BLE_UUID_MOTION_SERVICE;
    VERIFY_SUCCESS(err_code);
	
    // Add Custom Service UUID
    ble_uuid_t orientation_uuid;	
    ble_uuid128_t orientation_base_uuid = {BLE_UUID_ORIENTATION_CHAR_BASE};
    err_code =  sd_ble_uuid_vs_add(&orientation_base_uuid, &orientation_uuid.type);
	
    p_ble_motion_c->evt_handler                 = p_ble_motion_c_init->evt_handler;
    p_ble_motion_c->conn_handle                 = BLE_CONN_HANDLE_INVALID;
    p_ble_motion_c->peer_motion_db.motionm_cccd_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_motion_c->peer_motion_db.motionm_handle      = BLE_GATT_HANDLE_INVALID;


    
    return ble_db_discovery_evt_register(&motion_uuid);
}

uint32_t motion_command_write(ble_motion_c_t * p_motion_c, uint8_t * payload, uint8_t len)
{
    if (p_motion_c->peer_motion_db.motionm_handle == BLE_GATT_HANDLE_INVALID) {
        return BLE_ERROR_INVALID_ATTR_HANDLE;
    }
    
    
	NRF_LOG_INFO("Preamble: 0x%x", payload[0]);
	NRF_LOG_INFO("Command: 0x%x",  payload[1]);
	NRF_LOG_INFO("Length: %d", payload[2]);
	if (payload[2] > 0) {
		NRF_LOG_INFO("Arguments:");	
		NRF_LOG_HEXDUMP_INFO(&payload[3],payload[2]);
	}

	ble_gattc_write_params_t gattc_write = {
		.write_op = BLE_GATT_OP_WRITE_CMD,
		.flags = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_CANCEL,
		.handle = p_motion_c->peer_motion_db.motionm_handle,
		.offset = 0,
		.len = len,
		.p_value = payload,
	};
		
	//send value
	NRF_LOG_DEBUG("Sending Command on Connection Handle 0x%x, Command Handle 0x%x",p_motion_c->conn_handle, p_motion_c->peer_motion_db.motionm_handle);
	return sd_ble_gattc_write(p_motion_c->conn_handle, &gattc_write);
}


void ble_motion_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
     ble_motion_c_t * p_ble_motion_c = (ble_motion_c_t *)p_context;

    if ((p_ble_motion_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_motion_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_ble_motion_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnected(p_ble_motion_c, p_ble_evt);
            break;

        default:
            break;
    }
}

static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
    NRF_LOG_DEBUG("Configuring CCCD. CCCD Handle = %d, Connection Handle = %d",
        handle_cccd,conn_handle);

    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB_16(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB_16(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}


uint32_t ble_motion_c_motionm_notif_enable(ble_motion_c_t * p_ble_motion_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_motion_c);

    return cccd_configure(p_ble_motion_c->conn_handle,
                          p_ble_motion_c->peer_motion_db.motionm_cccd_handle,
                          true);
}


uint32_t ble_motion_c_handles_assign(ble_motion_c_t * p_ble_motion_c,
                                  uint16_t conn_handle,
                                  const motion_db_t * p_peer_motion_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_motion_c);

    p_ble_motion_c->conn_handle = conn_handle;
    if (p_peer_motion_handles != NULL)
    {
        p_ble_motion_c->peer_motion_db = *p_peer_motion_handles;
    }
    return NRF_SUCCESS;
}

