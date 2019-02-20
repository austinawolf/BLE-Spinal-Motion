


#ifndef BLE_MOTION_H__
#define BLE_MOTION_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"


//sds
#include "sds_error.h"
#include "sds_motion_types.h"
#include "sds_ble_motion_types.h"

/**@brief   Macro for defining a ble_motion instance.
 *
 * @param   _name   Name of the instance.
 * @hideinitializer
 */
#define BLE_MOTION_DEF(_name)                                                                          \
ble_motion_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_MOTION_BLE_OBSERVER_PRIO,                                                     \
                     ble_motion_on_ble_evt, &_name)

#include "sds_error.h"


typedef enum
{
    SESSION_IDLE,
	SESSION_TO_CENTRAL,    
	SESSION_TO_MEMORY,
	SESSION_TO_MEMORY_AND_CENTRAL,
	MIN_SESSION_VAL = SESSION_TO_CENTRAL,
	MAX_SESSION_VAL = SESSION_TO_MEMORY_AND_CENTRAL,	
} sds_session_destination_t;


/**@brief Motion Service event type. */
typedef enum
{
    BLE_MOTION_EVT_NOTIFICATION_ENABLED,   /**< Motion value notification enabled event. */
    BLE_MOTION_EVT_NOTIFICATION_DISABLED   /**< Motion value notification disabled event. */
} ble_motion_evt_type_t;

/**@brief Motion Service event. */
typedef struct
{
    ble_motion_evt_type_t evt_type;    /**< Type of event. */
} ble_motion_evt_t;

// Forward declaration of the ble_motion_t type.
typedef struct ble_motion_s ble_motion_t;

/**@brief Motion Service event handler type. */
typedef void (*ble_motion_command_evt_handler_t) (sds_command_t * p_command);

/**@brief Motion Service init structure. This contains all options and data needed for
 *        initialization of the service. */
typedef struct
{
    ble_motion_command_evt_handler_t	command_evt_handler;                                  /**< Event handler to be called for handling events in the Motion Service. */
    bool                         		is_sensor_contact_supported;                          /**< Determines if sensor contact detection is to be supported. */
    uint8_t *                    		p_body_sensor_location;                               /**< If not NULL, initial value of the Body Sensor Location characteristic. */
    ble_srv_cccd_security_mode_t 		motion_motionm_attr_md;                               /**< Initial security level for Motion service measurement attribute */
    ble_srv_security_mode_t      		motion_bsl_attr_md;                                   /**< Initial security level for body sensor location attribute */
} ble_motion_init_t;

/**@brief Motion Service structure. This contains various status information for the service. */
struct ble_motion_s
{
    ble_motion_command_evt_handler_t     command_evt_handler;                                  /**< Event handler to be called for handling events in the Motion Service. */
    bool                         is_expended_energy_supported;                         /**< TRUE if Expended Energy measurement is supported. */
    bool                         is_sensor_contact_supported;                          /**< TRUE if sensor contact detection is supported. */
    uint16_t                     service_handle;                                       /**< Handle of Motion Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t     motionm_handles;                                      /**< Handles related to the Motion Measurement characteristic. */
	ble_gatts_char_handles_t	 command_handles;
    ble_gatts_char_handles_t     bsl_handles;                                          /**< Handles related to the Body Sensor Location characteristic. */
    ble_gatts_char_handles_t     hrcp_handles;                                         /**< Handles related to the Motion Control Point characteristic. */
    uint16_t                     conn_handle;                                          /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    bool                         is_sensor_contact_detected;                           /**< TRUE if sensor contact has been detected. */
    uint8_t                      max_motionm_len;
	uint8_t						 uuid_type;
	/**< Current maximum HR measurement length, adjusted according to the current ATT MTU. */
};


/**@brief Function for initializing the Motion Service.
 *
 * @param[out]  p_motion       Motion Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_motion_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_motion_init(ble_motion_t * p_motion, ble_motion_init_t const * p_motion_init);


/**@brief Function for handling the GATT module's events.
 *
 * @details Handles all events from the GATT module of interest to the Motion Service.
 *
 * @param[in]   p_motion      Motion Service structure.
 * @param[in]   p_gatt_evt  Event received from the GATT module.
 */
void ble_motion_on_gatt_evt(ble_motion_t * p_motion, nrf_ble_gatt_evt_t const * p_gatt_evt);


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Motion Service.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 * @param[in]   p_context   Motion Service structure.
 */
void ble_motion_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

/**@brief Function for setting the state of the Sensor Contact Supported bit.
 *
 * @param[in]   p_motion                        Motion Service structure.
 * @param[in]   is_sensor_contact_supported  New state of the Sensor Contact Supported bit.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_motion_sensor_contact_supported_set(ble_motion_t * p_motion, bool is_sensor_contact_supported);


/**@brief Function for setting the state of the Sensor Contact Detected bit.
 *
 * @param[in]   p_motion                        Motion Service structure.
 * @param[in]   is_sensor_contact_detected   TRUE if sensor contact is detected, FALSE otherwise.
 */
void ble_motion_sensor_contact_detected_update(ble_motion_t * p_motion, bool is_sensor_contact_detected);


/**@brief Function for setting the Body Sensor Location.
 *
 * @details Sets a new value of the Body Sensor Location characteristic. The new value will be sent
 *          to the client the next time the client reads the Body Sensor Location characteristic.
 *
 * @param[in]   p_motion                 Motion Service structure.
 * @param[in]   body_sensor_location  New Body Sensor Location.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_motion_body_sensor_location_set(ble_motion_t * p_motion, uint8_t body_sensor_location);

/**@brief Function for sending motion data if notification has been enabled.
 *
 * @details The application calls this function after having performed a Motion measurement.
 *          If notification has been enabled, the Motion measurement data is encoded and sent to
 *          the client.
 *
 * @param[in]   p_motion                 Motion Service structure.
 * @param[in]   motion               	 New Motion measurement.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
 
uint32_t ble_motion_data_send(ble_motion_t * p_motion, motion_sample_t * p_motion_sample);

/**@brief Function for writing a response to the command charactersitics
 *
 * @details The application calls this function after recieiving a command.
 *
 * @param[in]   p_motion                 Motion Service structure.
 * @param[in]   opcode               	 Opcode. @ref sds_opcode_t
 * @param[in]   arg_len               	 Length of arguments.
 * @param[in]   p_args               	 Pointer to arguments.
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
 
uint32_t ble_motion_command_response_send(ble_motion_t * p_motion, sds_opcode_t opcode, uint8_t arg_len, uint8_t * p_args);

/**@brief Function for writing a response to the command charactersitics
 *
 * @details The application calls this function after recieiving a command.
 *
 * @param[in]   p_motion                 Motion Service structure.
 * @param[in]   opcode               	 Opcode. @ref sds_opcode_t
 * @param[in]   err_code               	 sds_return_t.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
 
uint32_t ble_motion_command_error_send(ble_motion_t * p_motion, sds_opcode_t opcode, sds_return_t err_code);

/**@brief Function for sending session info to central
 *
 * @details The application calls this function when session info is found in memory.
 *
 * @param[in]   p_motion                 Motion Service structure.
 * @param[in]   p_session_info           Pointer to a session info structure
 * @param[in]   err_code               	 sds_return_t.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_motion_session_info_send(ble_motion_t * p_motion, session_info_t * p_session_info);

#ifdef __cplusplus
}
#endif

#endif // BLE_MOTION_H__

/** @} */
