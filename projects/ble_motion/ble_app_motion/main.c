/** @file */ 

/*! \mainpage My Personal Index Page
 *
 * \section intro_sec Introduction
 *
 * This is the introduction.
 *
 * \section install_sec Installation
 *
 * \subsection step1 Step 1: Opening the box
 *
 * etc...
 */
#include "main.h"
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "bsp_btn_ble.h"
#include "peer_manager.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "ble_conn_state.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_nvic.h"
#include "app_scheduler.h"
#include "app_timer.h"

//sds
#include "sds_twi.h"
#include "sds_motion.h"
#include "sds_ble_motion.h"
#include "sds_config.h"
#include "sds_clock.h"
#include "sds_ble_uuid.h"
#include "sds_memory.h"
#include "sds_log_config.h"
#include "sds_error.h"

#define NRF_LOG_MODULE_NAME main
#if SDS_MAIN_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL SDS_MAIN_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR SDS_MAIN_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR SDS_MAIN_CONFIG_DEBUG_COLOR
#else
#define NRF_LOG_LEVEL       0
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

BLE_MOTION_DEF(m_motion);                                           /**< Motion service instance. */
BLE_BAS_DEF(m_bas);                                                 /**< Structure used to identify the battery service. */
NRF_BLE_GATT_DEF(m_gatt);                                           /**< GATT module instance. */
BLE_ADVERTISING_DEF(m_advertising);                                 /**< Advertising module instance. */
APP_TIMER_DEF(m_streaming_timer_id);                         		/**< Quaternion Orientation timer. */

#define CHECK_ARGLEN(_N) if (p_command -> arg_len != _N) do { \
    ble_motion_command_error_send(&m_motion, p_command->opcode, SDS_INVALID_ARG_LEN); \
    break; \
    } while (0)

typedef enum {
	DISCONNECTED,
	CONNECTED,
} connection_status_t;

typedef struct {
	sds_session_destination_t session_dest;
	connection_status_t connection_status;
} main_control_block_t;
	
static main_control_block_t cb;

static void on_connect(void);
static void on_disconnect(void);
static void ble_motion_command_evt_handler(sds_command_t * p_command);
static void streaming_timer_event_handler(void * p_context);
static void streaming_timer_stop(void);
static void streaming_timer_start(void);
static void streaming_timer_init(void);

static uint16_t m_conn_handle         = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static ble_uuid_t m_adv_uuids[] =                                   /**< Universally unique service identifiers. */
{
    {BLE_UUID_MOTION_SERVICE,           	BLE_UUID_TYPE_BLE},
    {BLE_UUID_BATTERY_SERVICE,              BLE_UUID_TYPE_BLE},
    {BLE_UUID_DEVICE_INFORMATION_SERVICE,   BLE_UUID_TYPE_BLE}
};





/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
    }
    else
    {
        ret_code_t err_code;

        err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
    }
}




/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;

    switch (p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
        {
            NRF_LOG_INFO("Connected to a previously bonded device.");
        } break;

        case PM_EVT_CONN_SEC_SUCCEEDED:
        {
            NRF_LOG_INFO("Connection secured: role: %d, conn_handle: 0x%x, procedure: %d.",
                         ble_conn_state_role(p_evt->conn_handle),
                         p_evt->conn_handle,
                         p_evt->params.conn_sec_succeeded.procedure);
        } break;

        case PM_EVT_CONN_SEC_FAILED:
        {
            /* Often, when securing fails, it shouldn't be restarted, for security reasons.
             * Other times, it can be restarted directly.
             * Sometimes it can be restarted, but only after changing some Security Parameters.
             * Sometimes, it cannot be restarted until the link is disconnected and reconnected.
             * Sometimes it is impossible, to secure the link, or the peer device does not support it.
             * How to handle this error is highly application dependent. */
        } break;

        case PM_EVT_CONN_SEC_CONFIG_REQ:
        {
            // Reject pairing request from an already bonded peer.
            pm_conn_sec_config_t conn_sec_config = {.allow_repairing = false};
            pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
        } break;

        case PM_EVT_STORAGE_FULL:
        {
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code == FDS_ERR_BUSY || err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
            {
                // Retry.
            }
            else
            {
                APP_ERROR_CHECK(err_code);
            }
        } break;

        case PM_EVT_PEERS_DELETE_SUCCEEDED:
        {
            NRF_LOG_DEBUG("PM_EVT_PEERS_DELETE_SUCCEEDED");
            advertising_start(false);
        } break;

        case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
        {
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
        } break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_data_update_failed.error);
        } break;

        case PM_EVT_PEER_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peer_delete_failed.error);
        } break;

        case PM_EVT_PEERS_DELETE_FAILED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.peers_delete_failed_evt.error);
        } break;

        case PM_EVT_ERROR_UNEXPECTED:
        {
            // Assert.
            APP_ERROR_CHECK(p_evt->params.error_unexpected.error);
        } break;

        case PM_EVT_CONN_SEC_START:
        case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
        case PM_EVT_PEER_DELETE_SUCCEEDED:
        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
        case PM_EVT_SERVICE_CHANGED_IND_SENT:
        case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED:
        default:
            break;
    }
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief GATT module event handler.
 */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
	
    if (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED)
    {
        NRF_LOG_INFO("GATT ATT MTU on connection 0x%x changed to %d.",
                     p_evt->conn_handle,
                     p_evt->params.att_mtu_effective);
    }

    ble_motion_on_gatt_evt(&m_motion, p_evt);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
	
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
    ret_code_t     err_code;
    ble_motion_init_t motion_init;
    ble_bas_init_t bas_init;
    ble_dis_init_t dis_init;
    uint8_t        body_sensor_location;

    memset(&motion_init, 0, sizeof(motion_init));

    motion_init.command_evt_handler                 = ble_motion_command_evt_handler;
    motion_init.is_sensor_contact_supported 		= true;
    motion_init.p_body_sensor_location      		= &body_sensor_location;

    // Here the sec level for the Heart Rate Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&motion_init.motion_motionm_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&motion_init.motion_motionm_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&motion_init.motion_motionm_attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&motion_init.motion_bsl_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&motion_init.motion_bsl_attr_md.write_perm);

    err_code = ble_motion_init(&m_motion, &motion_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Battery Service.
    memset(&bas_init, 0, sizeof(bas_init));

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    // Here the sec level for the Battery Service can be changed/increased.
    bas_init.bl_rd_sec        = SEC_OPEN;
    bas_init.bl_cccd_wr_sec   = SEC_OPEN;
    bas_init.bl_report_rd_sec = SEC_OPEN;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);

    // Initialize Device Information Service.
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);

    dis_init.dis_char_rd_sec = SEC_OPEN;

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}






/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = m_motion.motionm_handles.cccd_handle;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}




/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            break;

        default:
            break;
    }
}

/**@brief Function for handling BLE Motion Commands.
 *
 * @param[in]   p_command   BLE command structure.
 */
static void ble_motion_command_evt_handler(sds_command_t * p_command) {
	NRF_LOG_INFO("Motion Command Event. Opcode: 0x%x", p_command->opcode);
	
	sds_return_t err_code;
	
	static motion_rate_e motion_rate;
    static sds_session_destination_t session_destination;
    static uint8_t number_of_samples;
    session_info_t session_info;
	
	//const uint8_t resp_arg_len = 4;	
    switch (p_command->opcode)
    {
        case GET_FW_VERSION:
            CHECK_ARGLEN(0);
        
			//static uint8_t fw_version[resp_arg_len] = {0x0a, 0x0b , 0x0c, 0x0d};	   
			ble_motion_command_response_send(&m_motion, p_command->opcode, 0, NULL);	
			break;

        case GET_SAMPLE_RATE:
			CHECK_ARGLEN(0);
        
			motion_rate = sds_motion_get_rate();  			
			ble_motion_command_response_send(&m_motion, p_command->opcode, 1, &motion_rate);			
			break;	

        case SET_SAMPLE_RATE:
			CHECK_ARGLEN(1);
        
			motion_rate = (motion_rate_e) p_command -> p_args[0];		

			sds_motion_set_rate(motion_rate);	
			ble_motion_command_response_send(&m_motion, p_command->opcode, 0, NULL);          
			break;
			
        case RUN_MOTION_CAL:
			CHECK_ARGLEN(0);
        
            sds_motion_run_calibration();
			ble_motion_command_response_send(&m_motion, p_command->opcode, 0, NULL);			
			break;

        case START_SESSION:
			CHECK_ARGLEN(1);
        
			/* decode session destination argument */
			session_destination = (sds_session_destination_t) p_command -> p_args[0];
            
			/* get motion session info */
			get_session_info(&session_info);
		
            /* check for valid argument */
            if (session_destination > MAX_SESSION_VAL) {
                ble_motion_command_error_send(&m_motion, p_command->opcode, SDS_INVALID_ARG);                
            }
            else {
                cb.session_dest = session_destination;
            }
			
            /* if starting a memory session, prep the memory. Note: the session start memory event will start the memory */
            if (session_destination == SESSION_TO_MEMORY || session_destination == SESSION_TO_MEMORY_AND_CENTRAL) {              
                sds_mem_start_session(&session_info);
            }    
            else {
                /* if starting a central session only, start sampling immediately */
                sds_motion_set_state(SAMPLE_ON);
            }
               
			/* Send response notificaiton with session info data */
			ble_motion_command_response_send(&m_motion, p_command->opcode, sizeof(session_info_t), (uint8_t *) &session_info);						
			break;
        
        case STOP_SESSION:
			CHECK_ARGLEN(0);
        
            cb.session_dest = SESSION_IDLE;
			sds_motion_set_state(SAMPLE_OFF);
            sds_mem_stop_session();
                    
			ble_motion_command_response_send(&m_motion, p_command->opcode, 0, NULL);						
			break;

        case START_MEMORY_STREAM:
			CHECK_ARGLEN(0);
            
			streaming_timer_start();
            err_code = sds_mem_start_stream(&session_info);
            if (err_code) {
                ble_motion_command_error_send(&m_motion, p_command->opcode, err_code);   
                break;
            }
                            
			ble_motion_command_response_send(&m_motion, p_command->opcode, sizeof(session_info_t), (uint8_t *) &session_info);						
			break;

        case REQUEST_STREAM_DATA:
			CHECK_ARGLEN(1);
            
            number_of_samples = p_command -> p_args[0];
        
            err_code = sds_stream_request_samples(number_of_samples);
            if (err_code) {
                ble_motion_command_error_send(&m_motion, p_command->opcode, err_code);
                break;
            }
                    
			ble_motion_command_response_send(&m_motion, p_command->opcode, 0, NULL);						
			break; 
            
        case STOP_MEMORY_STREAM:
			CHECK_ARGLEN(0);
        
			streaming_timer_stop();
            err_code = sds_mem_stop_stream();
            if (err_code) {
                ble_motion_command_error_send(&m_motion, p_command->opcode, err_code);
                break;
            }
                    
			ble_motion_command_response_send(&m_motion, p_command->opcode, 0, NULL);						
			break;
             
        default:
            NRF_LOG_WARNING("Unhandled Command");
			ble_motion_command_error_send(&m_motion, p_command->opcode, SDS_UNHANDLED_COMMAND);
            break;
    }
	
}

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;
	
    switch (p_ble_evt->header.evt_id)
    {
		case BLE_GATTS_EVT_HVN_TX_COMPLETE:
			NRF_LOG_DEBUG("Notifications Sent:%d", p_ble_evt->evt.gatts_evt.params.hvn_tx_complete.count);
    	
        case BLE_GAP_EVT_CONNECTED:
			on_connect();
		
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
			cb.connection_status = CONNECTED;
            break;

        case BLE_GAP_EVT_DISCONNECTED:

			on_disconnect();
		
            NRF_LOG_INFO("Disconnected, reason %d.",p_ble_evt->evt.gap_evt.params.disconnected.reason);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
			cb.connection_status = DISCONNECTED;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;
    
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            NRF_LOG_DEBUG("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;
        
        case BLE_GAP_EVT_AUTH_KEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_AUTH_KEY_REQUEST");
            break;

        case BLE_GAP_EVT_LESC_DHKEY_REQUEST:
            NRF_LOG_INFO("BLE_GAP_EVT_LESC_DHKEY_REQUEST");
            break;

         case BLE_GAP_EVT_AUTH_STATUS:
             NRF_LOG_INFO("BLE_GAP_EVT_AUTH_STATUS: status=0x%x bond=0x%x lv4: %d kdist_own:0x%x kdist_peer:0x%x",
                          p_ble_evt->evt.gap_evt.params.auth_status.auth_status,
                          p_ble_evt->evt.gap_evt.params.auth_status.bonded,
                          p_ble_evt->evt.gap_evt.params.auth_status.sm1_levels.lv4,
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_own),
                          *((uint8_t *)&p_ble_evt->evt.gap_evt.params.auth_status.kdist_peer));
            break;

        default:
            // No implementation needed.
            break;
    }
}



/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	
	
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);

}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance      = true;
    init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    switch (event)
    {
        default:
            break;
    }
}

/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    //*p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for handling the Heart rate measurement timer timeout.
 *
 * @details This function will be called each time the heart rate measurement timer expires.
 *          It will exclude RR Interval data from every third measurement.
 *
 * @param[in] p_context  Pointer used for passing some arbitrary information (context) from the
 *                       app_start_timer() call to the timeout handler.
 */

static void motion_sample_handler(void * p_context)
{	
    ret_code_t      err_code = NRF_SUCCESS;
    UNUSED_PARAMETER(p_context);

	motion_sample_t * p_motion_sample;
	p_motion_sample = p_context;
	NRF_LOG_DEBUG("Motion Handler Call Event: %d", p_motion_sample->event);
	
	if (cb.session_dest == SESSION_TO_CENTRAL || cb.session_dest == SESSION_TO_MEMORY_AND_CENTRAL) {
		
		if (cb.connection_status == CONNECTED) {
			err_code = ble_motion_data_send(&m_motion, p_motion_sample);
			APP_ERROR_CHECK(err_code);
		}
		
	}
	
    if (cb.session_dest == SESSION_TO_MEMORY || cb.session_dest == SESSION_TO_MEMORY_AND_CENTRAL) {
		err_code = sds_mem_save_motion_data(p_motion_sample);
		APP_ERROR_CHECK(err_code);
	}
}


static void on_connect(void) {	
		
}

static void on_disconnect(void) {
	
}

void memory_event_handler(sds_memory_evt_t event, void * p_context) {

    motion_sample_t * p_motion_sample;
    sds_return_t err_code;
	uint32_t nrf_err_code;
    
    switch (event) {
    
        case(SDS_MEMORY_EVT_SESSION_STARTED):
            err_code = sds_motion_set_state(SAMPLE_ON);
            APP_ERROR_CHECK(err_code);
        
            NRF_LOG_INFO("Memory session started.");
            break;
        
        case(SDS_MEMORY_EVT_FLASH_FULL):
            NRF_LOG_INFO("Flash Memory Full.");        
            break;   
        
        case(SDS_MEMORY_EVT_SESSION_TERMINATED):
            err_code = sds_motion_set_state(SAMPLE_OFF);
            APP_ERROR_CHECK(err_code);			
            NRF_LOG_INFO("Memory session terminated.");
            break;
		
        case(SDS_MEMORY_EVT_STREAM_TERMINATED):
			streaming_timer_stop();
            NRF_LOG_INFO("Memory stream terminated.");
            break;
        
        case(SDS_MEMORY_EVT_STREAM_STARTED):
            NRF_LOG_INFO("Memory stream started.");

            break; 
        
        case(SDS_MEMORY_EVT_MOTION_DATA):
            
            p_motion_sample =  p_context;

            NRF_LOG_DEBUG("New Motion Data.");      
            NRF_LOG_DEBUG("q0=%ld, q1=%ld, q3=%ld, q4=%ld.",p_motion_sample->quat[0], p_motion_sample->quat[1], p_motion_sample->quat[2], p_motion_sample->quat[3]);        
            NRF_LOG_DEBUG("ax=%d, ay=%d, az=%d",p_motion_sample->accel[0], p_motion_sample->accel[1], p_motion_sample->accel[2]);        
            NRF_LOG_DEBUG("gx=%d, gy=%d, gz=%d",p_motion_sample->gyro[0], p_motion_sample->gyro[1], p_motion_sample->gyro[2]);        
            NRF_LOG_DEBUG("cx=%d, cy=%d, cz=%d",p_motion_sample->compass[0], p_motion_sample->compass[1], p_motion_sample->compass[2]);        

			nrf_err_code = ble_motion_data_send(&m_motion, p_motion_sample);		
			APP_ERROR_CHECK(nrf_err_code);

			break;       
    }
    
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void streaming_timer_init(void)
{
    ret_code_t err_code;
    err_code = app_timer_create(&m_streaming_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                streaming_timer_event_handler);
    APP_ERROR_CHECK(err_code);
}

static void streaming_timer_start(void)
{
    ret_code_t err_code;
	NRF_LOG_DEBUG("Streaming timers start.");

    err_code = app_timer_start(m_streaming_timer_id, APP_TIMER_TICKS(CONN_INTERVAL_MS), NULL);
    APP_ERROR_CHECK(err_code);
}

static void streaming_timer_stop(void)
{
	ret_code_t err_code;
	NRF_LOG_DEBUG("Streaming timers stop.");
	
    err_code = app_timer_stop(m_streaming_timer_id);
    APP_ERROR_CHECK(err_code);
}

static void streaming_timer_event_handler(void * p_context) {
	#define TO_MSEC(CONN_INT) CONN_INT / 1.25
	
	ret_code_t err_code;
	
	NRF_LOG_DEBUG("Streaming Timer Expired");
	
	err_code = sds_stream_request_samples(1);
	if (err_code) {
		NRF_LOG_ERROR("Invalid State Error.");
	}
}

/** My function doing something...
    @param param1 first parameter
    @param param2 second parameter
    @return value return value
*/
int main(void)
{
    bool erase_bonds;
	uint32_t err_code = NRF_SUCCESS;
	
	/* NRF Logger Init */
    log_init();
	
	/* TWI Init */
	sds_twi_init();
	
	/* Time Module Init */
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
	
	/* Board Init */
	buttons_leds_init(&erase_bonds);

	/* BLE Support Init */
    ble_stack_init();	
    gap_params_init();	
    gatt_init();
    advertising_init();
    services_init();
    conn_params_init();
    peer_manager_init();
	
	/* Motion Init */
	motion_init_t motion_init = {
		.event_callback = motion_sample_handler,		
		.motion_rate = (motion_rate_e) SDS_MOTION_DEFAULT_SAMPLE_RATE,
		.compass_rate = (motion_rate_e) SDS_COMPASS_DEFAULT_SAMPLE_RATE,
		.motion_mode = (motion_mode_e) SDS_MOTION_CONFIG_MODE,		
	};	
	err_code = sds_motion_init(&motion_init);	
	APP_ERROR_CHECK(err_code);
	
	/* Memory Init */
    memory_init_s memory_init = {
		.event_callback = memory_event_handler,
    };
    sds_mem_init(&memory_init);	
	
	/* Streaming Timer Init */
	streaming_timer_init();

	/* Power Management Init */	
	nrf_pwr_mgmt_init();
	
	/* Start Execution */
    NRF_LOG_INFO("Orientation Sensor example started.");
    advertising_start(true);

    /* Main Loop */
    for (;;)
    {
        if (NRF_LOG_PROCESS() == false)
        {
			app_sched_execute();
            nrf_pwr_mgmt_run();
        }
    }
}


