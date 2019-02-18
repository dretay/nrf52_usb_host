#include "nrf_services.h"

static bool              m_in_boot_mode = false; /**< Current protocol mode. */
NRF_BLE_QWR_DEF(m_qwr); /**< Context for the Queued Write module.*/

BLE_HIDS_DEF(m_hids,
	/**< HID service instance. */
             NRF_SDH_BLE_TOTAL_LINK_COUNT,
	INPUT_REP_BUTTONS_LEN,
	INPUT_REP_MOVEMENT_LEN,
	INPUT_REP_MEDIA_PLAYER_LEN);

extern uint16_t m_conn_handle;

/**@brief Function for initializing Device Information Service.
 */
static void dis_init(void)
{
	ret_code_t       err_code;
	ble_dis_init_t   dis_init_obj;
	ble_dis_pnp_id_t pnp_id;

	pnp_id.vendor_id_source = PNP_ID_VENDOR_ID_SOURCE;
	pnp_id.vendor_id        = PNP_ID_VENDOR_ID;
	pnp_id.product_id       = PNP_ID_PRODUCT_ID;
	pnp_id.product_version  = PNP_ID_PRODUCT_VERSION;

	memset(&dis_init_obj, 0, sizeof(dis_init_obj));

	ble_srv_ascii_to_utf8(&dis_init_obj.manufact_name_str, MANUFACTURER_NAME);
	dis_init_obj.p_pnp_id = &pnp_id;

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&dis_init_obj.dis_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init_obj.dis_attr_md.write_perm);

	err_code = ble_dis_init(&dis_init_obj);
	APP_ERROR_CHECK(err_code);
}
/**@brief Function for handling HID events.
 *
 * @details This function will be called for all HID events which are passed to the application.
 *
 * @param[in]   p_hids  HID service structure.
 * @param[in]   p_evt   Event received from the HID service.
 */
static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt)
{
	switch (p_evt->evt_type)
	{
	case BLE_HIDS_EVT_BOOT_MODE_ENTERED:
		m_in_boot_mode = true;
		break;

	case BLE_HIDS_EVT_REPORT_MODE_ENTERED:
		m_in_boot_mode = false;
		break;

	case BLE_HIDS_EVT_NOTIF_ENABLED:
		break;

	default:
		// No implementation needed.
		break;
	}
}
/**@brief Function for sending a Mouse Movement.
 *
 * @param[in]   x_delta   Horizontal movement.
 * @param[in]   y_delta   Vertical movement.
 */
static void mouse_movement_send(int16_t x_delta, int16_t y_delta)
{
	ret_code_t err_code;

	if (m_in_boot_mode)
	{
		x_delta = MIN(x_delta, 0x00ff);
		y_delta = MIN(y_delta, 0x00ff);

		err_code = ble_hids_boot_mouse_inp_rep_send(&m_hids,
			0x00,
			(int8_t)x_delta,
			(int8_t)y_delta,
			0,
			NULL,
			m_conn_handle);
	}
	else
	{
		uint8_t buffer[INPUT_REP_MOVEMENT_LEN];

		APP_ERROR_CHECK_BOOL(INPUT_REP_MOVEMENT_LEN == 3);

		x_delta = MIN(x_delta, 0x0fff);
		y_delta = MIN(y_delta, 0x0fff);

		buffer[0] = x_delta & 0x00ff;
		buffer[1] = ((y_delta & 0x000f) << 4) | ((x_delta & 0x0f00) >> 8);
		buffer[2] = (y_delta & 0x0ff0) >> 4;

		err_code = ble_hids_inp_rep_send(&m_hids,
			INPUT_REP_MOVEMENT_INDEX,
			INPUT_REP_MOVEMENT_LEN,
			buffer,
			m_conn_handle);
	}

	if ((err_code != NRF_SUCCESS) &&
	    (err_code != NRF_ERROR_INVALID_STATE) &&
	    (err_code != NRF_ERROR_RESOURCES) &&
	    (err_code != NRF_ERROR_BUSY) &&
	    (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING))
	{
		APP_ERROR_HANDLER(err_code);
	}
}
/**@brief Function for handling Service errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}
/**@brief Function for initializing HID Service.
 */
static void hids_init(void)
{
	ret_code_t                err_code;
	ble_hids_init_t           hids_init_obj;
	ble_hids_inp_rep_init_t * p_input_report;
	uint8_t                   hid_info_flags;

	static ble_hids_inp_rep_init_t inp_rep_array[INPUT_REPORT_COUNT];
	static uint8_t rep_map_data[] =
	{
		0x05,
		0x01,
		  // Usage Page (Generic Desktop)
		0x09,
		0x02,
		  // Usage (Mouse)

		0xA1,
		0x01, 
		 // Collection (Application)

		// Report ID 1: Mouse buttons + scroll/pan
		0x85,
		0x01, 
		       // Report Id 1
		0x09,
		0x01, 
		       // Usage (Pointer)
		0xA1,
		0x00,
		        // Collection (Physical)
		0x95,
		0x05,
		        // Report Count (3)
		0x75,
		0x01, 
		       // Report Size (1)
		0x05,
		0x09,
		        // Usage Page (Buttons)
		0x19,
		0x01,
		        // Usage Minimum (01)
		0x29,
		0x05,
		        // Usage Maximum (05)
		0x15,
		0x00,
		        // Logical Minimum (0)
		0x25,
		0x01, 
		       // Logical Maximum (1)
		0x81,
		0x02,
		        // Input (Data, Variable, Absolute)
		0x95,
		0x01,
		        // Report Count (1)
		0x75,
		0x03,
		        // Report Size (3)
		0x81,
		0x01, 
		       // Input (Constant) for padding
		0x75,
		0x08,
		        // Report Size (8)
		0x95,
		0x01, 
		       // Report Count (1)
		0x05,
		0x01,
		        // Usage Page (Generic Desktop)
		0x09,
		0x38, 
		       // Usage (Wheel)
		0x15,
		0x81,
		        // Logical Minimum (-127)
		0x25,
		0x7F,
		        // Logical Maximum (127)
		0x81,
		0x06, 
		       // Input (Data, Variable, Relative)
		0x05,
		0x0C,
		        // Usage Page (Consumer)
		0x0A,
		0x38,
		0x02, 
		 // Usage (AC Pan)
		0x95,
		0x01,
		        // Report Count (1)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0xC0,
		              // End Collection (Physical)

		// Report ID 2: Mouse motion
		0x85,
		0x02,
		        // Report Id 2
		0x09,
		0x01,
		        // Usage (Pointer)
		0xA1,
		0x00, 
		       // Collection (Physical)
		0x75,
		0x0C,
		        // Report Size (12)
		0x95,
		0x02,
		        // Report Count (2)
		0x05,
		0x01, 
		       // Usage Page (Generic Desktop)
		0x09,
		0x30,
		        // Usage (X)
		0x09,
		0x31,
		        // Usage (Y)
		0x16,
		0x01,
		0xF8, 
		 // Logical maximum (2047)
		0x26,
		0xFF,
		0x07,
		  // Logical minimum (-2047)
		0x81,
		0x06,
		        // Input (Data, Variable, Relative)
		0xC0,
		              // End Collection (Physical)
		0xC0,
		              // End Collection (Application)

		// Report ID 3: Advanced buttons
		0x05,
		0x0C, 
		       // Usage Page (Consumer)
		0x09,
		0x01,
		        // Usage (Consumer Control)
		0xA1,
		0x01,
		        // Collection (Application)
		0x85,
		0x03,
		        // Report Id (3)
		0x15,
		0x00, 
		       // Logical minimum (0)
		0x25,
		0x01,
		        // Logical maximum (1)
		0x75,
		0x01,
		        // Report Size (1)
		0x95,
		0x01,
		        // Report Count (1)

		0x09,
		0xCD,
		        // Usage (Play/Pause)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0x0A,
		0x83,
		0x01, 
		 // Usage (AL Consumer Control Configuration)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0x09,
		0xB5, 
		       // Usage (Scan Next Track)
		0x81,
		0x06, 
		       // Input (Data,Value,Relative,Bit Field)
		0x09,
		0xB6,
		        // Usage (Scan Previous Track)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)

		0x09,
		0xEA, 
		       // Usage (Volume Down)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0x09,
		0xE9, 
		       // Usage (Volume Up)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0x0A,
		0x25,
		0x02, 
		 // Usage (AC Forward)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0x0A,
		0x24,
		0x02,
		  // Usage (AC Back)
		0x81,
		0x06,
		        // Input (Data,Value,Relative,Bit Field)
		0xC0              // End Collection
	};

	memset(inp_rep_array, 0, sizeof(inp_rep_array));
	// Initialize HID Service.
	p_input_report                      = &inp_rep_array[INPUT_REP_BUTTONS_INDEX];
	p_input_report->max_len             = INPUT_REP_BUTTONS_LEN;
	p_input_report->rep_ref.report_id   = INPUT_REP_REF_BUTTONS_ID;
	p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

	p_input_report                      = &inp_rep_array[INPUT_REP_MOVEMENT_INDEX];
	p_input_report->max_len             = INPUT_REP_MOVEMENT_LEN;
	p_input_report->rep_ref.report_id   = INPUT_REP_REF_MOVEMENT_ID;
	p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

	p_input_report                      = &inp_rep_array[INPUT_REP_MPLAYER_INDEX];
	p_input_report->max_len             = INPUT_REP_MEDIA_PLAYER_LEN;
	p_input_report->rep_ref.report_id   = INPUT_REP_REF_MPLAYER_ID;
	p_input_report->rep_ref.report_type = BLE_HIDS_REP_TYPE_INPUT;

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&p_input_report->security_mode.write_perm);

	hid_info_flags = HID_INFO_FLAG_REMOTE_WAKE_MSK | HID_INFO_FLAG_NORMALLY_CONNECTABLE_MSK;

	memset(&hids_init_obj, 0, sizeof(hids_init_obj));

	hids_init_obj.evt_handler                    = on_hids_evt;
	hids_init_obj.error_handler                  = service_error_handler;
	hids_init_obj.is_kb                          = false;
	hids_init_obj.is_mouse                       = true;
	hids_init_obj.inp_rep_count                  = INPUT_REPORT_COUNT;
	hids_init_obj.p_inp_rep_array                = inp_rep_array;
	hids_init_obj.outp_rep_count                 = 0;
	hids_init_obj.p_outp_rep_array               = NULL;
	hids_init_obj.feature_rep_count              = 0;
	hids_init_obj.p_feature_rep_array            = NULL;
	hids_init_obj.rep_map.data_len               = sizeof(rep_map_data);
	hids_init_obj.rep_map.p_data                 = rep_map_data;
	hids_init_obj.hid_information.bcd_hid        = BASE_USB_HID_SPEC_VERSION;
	hids_init_obj.hid_information.b_country_code = 0;
	hids_init_obj.hid_information.flags          = hid_info_flags;
	hids_init_obj.included_services_count        = 0;
	hids_init_obj.p_included_services_array      = NULL;

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.rep_map.security_mode.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.rep_map.security_mode.write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.hid_information.security_mode.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.hid_information.security_mode.write_perm);

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
	    &hids_init_obj.security_mode_boot_mouse_inp_rep.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
	    &hids_init_obj.security_mode_boot_mouse_inp_rep.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(
	    &hids_init_obj.security_mode_boot_mouse_inp_rep.write_perm);

	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_protocol.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_protocol.write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&hids_init_obj.security_mode_ctrl_point.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_ENC_NO_MITM(&hids_init_obj.security_mode_ctrl_point.write_perm);

	err_code = ble_hids_init(&m_hids, &hids_init_obj);
	APP_ERROR_CHECK(err_code);
}
/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}
/**@brief Function for initializing the Queued Write Module.
 */
static void qwr_init(void)
{
	ret_code_t         err_code;
	nrf_ble_qwr_init_t qwr_init_obj = { 0 };

	qwr_init_obj.error_handler = nrf_qwr_error_handler;

	err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init_obj);
	APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
	qwr_init();
	dis_init();
	NRF_Battery.bas_init();
	hids_init();
}


const struct nrf_services NRF_Services = { 
	.dis_init = dis_init,
	.on_hids_evt = on_hids_evt,
	.services_init = services_init,
	.mouse_movement_send = mouse_movement_send
	
};
