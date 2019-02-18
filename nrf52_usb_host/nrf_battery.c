#include "nrf_battery.h"

static sensorsim_cfg_t   m_battery_sim_cfg; /**< Battery Level sensor simulator configuration. */
static sensorsim_state_t m_battery_sim_state; /**< Battery Level sensor simulator state. */
BLE_BAS_DEF(m_bas); /**< Battery service instance. */


/**@brief Function for performing a battery measurement, and update the Battery Level characteristic in the Battery Service.
 */
static void battery_level_update(void)
{
	ret_code_t err_code;
	uint8_t  battery_level;

	battery_level = (uint8_t)sensorsim_measure(&m_battery_sim_state, &m_battery_sim_cfg);

	err_code = ble_bas_battery_level_update(&m_bas, battery_level, BLE_CONN_HANDLE_ALL);
	if ((err_code != NRF_SUCCESS) &&
	    (err_code != NRF_ERROR_BUSY) &&
	    (err_code != NRF_ERROR_RESOURCES) &&
	    (err_code != NRF_ERROR_FORBIDDEN) &&
	    (err_code != NRF_ERROR_INVALID_STATE) &&
	    (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
	{
		APP_ERROR_HANDLER(err_code);
	}
}
/**@brief Function for handling the Battery measurement timer timeout.
 *
 * @details This function will be called each time the battery level measurement timer expires.
 *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void battery_level_meas_timeout_handler(void * p_context)
{
	UNUSED_PARAMETER(p_context);
	battery_level_update();
}
/**@brief Function for initializing Battery Service.
 */
static void bas_init(void)
{
	ret_code_t     err_code;
	ble_bas_init_t bas_init_obj;

	memset(&bas_init_obj, 0, sizeof(bas_init_obj));

	bas_init_obj.evt_handler          = NULL;
	bas_init_obj.support_notification = true;
	bas_init_obj.p_report_ref         = NULL;
	bas_init_obj.initial_batt_level   = 100;

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_char_attr_md.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_char_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init_obj.battery_level_char_attr_md.write_perm);

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&bas_init_obj.battery_level_report_read_perm);

	err_code = ble_bas_init(&m_bas, &bas_init_obj);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the battery sensor simulator.
 */
static void sensor_simulator_init(void)
{
	m_battery_sim_cfg.min          = MIN_BATTERY_LEVEL;
	m_battery_sim_cfg.max          = MAX_BATTERY_LEVEL;
	m_battery_sim_cfg.incr         = BATTERY_LEVEL_INCREMENT;
	m_battery_sim_cfg.start_at_max = true;

	sensorsim_init(&m_battery_sim_state, &m_battery_sim_cfg);
}

const struct nrf_battery NRF_Battery= { 
	.battery_level_update = battery_level_update,
	.battery_level_meas_timeout_handler = battery_level_meas_timeout_handler,
	.bas_init = bas_init,
	.sensor_simulator_init = sensor_simulator_init,
	
};
