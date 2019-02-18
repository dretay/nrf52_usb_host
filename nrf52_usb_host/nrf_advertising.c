#include "nrf_advertising.h"

static pm_peer_id_t      m_peer_id; /**< Device reference handle to the current bonded central. */
static ble_uuid_t        m_adv_uuids[] =                                            /**< Universally unique service identifiers. */
{
	{ BLE_UUID_HUMAN_INTERFACE_DEVICE_SERVICE, BLE_UUID_TYPE_BLE }
};
static pm_peer_id_t      m_whitelist_peers[BLE_GAP_WHITELIST_ADDR_MAX_COUNT]; /**< List of peers currently in the whitelist. */
static uint32_t          m_whitelist_peer_cnt; /**< Number of peers currently in the whitelist. */

static uint32_t get_m_whitelist_peer_cnt(void)
{
	return m_whitelist_peer_cnt;
}
static pm_peer_id_t* get_m_whitelist_peers(void)
{
	return m_whitelist_peers;
}

static void add_m_whitelist_peer(pm_peer_id_t *m_peer_id_in)
{
	m_whitelist_peers[m_whitelist_peer_cnt++] = *m_peer_id_in;
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



/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
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
	case BLE_ADV_EVT_DIRECTED_HIGH_DUTY:
		NRF_LOG_INFO("Directed advertising.");
		err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_DIRECTED);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_ADV_EVT_FAST:
		NRF_LOG_INFO("Fast advertising.");
		err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_ADV_EVT_SLOW:
		NRF_LOG_INFO("Slow advertising.");
		err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_SLOW);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_ADV_EVT_FAST_WHITELIST:
		NRF_LOG_INFO("Fast advertising with whitelist.");
		err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_ADV_EVT_SLOW_WHITELIST:
		NRF_LOG_INFO("Slow advertising with whitelist.");
		err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING_WHITELIST);
		APP_ERROR_CHECK(err_code);
		err_code = ble_advertising_restart_without_whitelist(&m_advertising);
		APP_ERROR_CHECK(err_code);
		break;

	case BLE_ADV_EVT_IDLE:
		err_code = bsp_indication_set(BSP_INDICATE_IDLE);
		APP_ERROR_CHECK(err_code);
		NRF_Util.sleep_mode_enter();
		break;

	case BLE_ADV_EVT_WHITELIST_REQUEST:
		{
			ble_gap_addr_t whitelist_addrs[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
			ble_gap_irk_t  whitelist_irks[BLE_GAP_WHITELIST_ADDR_MAX_COUNT];
			uint32_t       addr_cnt = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;
			uint32_t       irk_cnt  = BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

			err_code = pm_whitelist_get(whitelist_addrs,
				&addr_cnt,
				whitelist_irks,
				&irk_cnt);
			APP_ERROR_CHECK(err_code);
			NRF_LOG_DEBUG("pm_whitelist_get returns %d addr in whitelist and %d irk whitelist",
				addr_cnt,
				irk_cnt);

			// Apply the whitelist.
			err_code = ble_advertising_whitelist_reply(&m_advertising,
				whitelist_addrs,
				addr_cnt,
				whitelist_irks,
				irk_cnt);
			APP_ERROR_CHECK(err_code);
		}
		break;

	case BLE_ADV_EVT_PEER_ADDR_REQUEST:
		{
			pm_peer_data_bonding_t peer_bonding_data;

			// Only Give peer address if we have a handle to the bonded peer.
			if(m_peer_id != PM_PEER_ID_INVALID)
			{

				err_code = pm_peer_data_bonding_load(m_peer_id, &peer_bonding_data);
				if (err_code != NRF_ERROR_NOT_FOUND)
				{
					APP_ERROR_CHECK(err_code);

					ble_gap_addr_t * p_peer_addr = &(peer_bonding_data.peer_ble_id.id_addr_info);
					err_code = ble_advertising_peer_addr_reply(&m_advertising, p_peer_addr);
					APP_ERROR_CHECK(err_code);
				}

			}
			break;
		}

	default:
		break;
	}
}
/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
	ret_code_t             err_code;
	uint8_t                adv_flags;
	ble_advertising_init_t init;

	memset(&init, 0, sizeof(init));

	adv_flags                            = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
	init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
	init.advdata.include_appearance      = true;
	init.advdata.flags                   = adv_flags;
	init.advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
	init.advdata.uuids_complete.p_uuids  = m_adv_uuids;

	init.config.ble_adv_whitelist_enabled          = true;
	init.config.ble_adv_directed_high_duty_enabled = true;
	init.config.ble_adv_directed_enabled           = false;
	init.config.ble_adv_directed_interval          = 0;
	init.config.ble_adv_directed_timeout           = 0;
	init.config.ble_adv_fast_enabled               = true;
	init.config.ble_adv_fast_interval              = APP_ADV_FAST_INTERVAL;
	init.config.ble_adv_fast_timeout               = APP_ADV_FAST_DURATION;
	init.config.ble_adv_slow_enabled               = true;
	init.config.ble_adv_slow_interval              = APP_ADV_SLOW_INTERVAL;
	init.config.ble_adv_slow_timeout               = APP_ADV_SLOW_DURATION;

	init.evt_handler   = on_adv_evt;
	init.error_handler = ble_advertising_error_handler;

	err_code = ble_advertising_init(&m_advertising, &init);
	APP_ERROR_CHECK(err_code);

	ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

/**@brief Fetch the list of peer manager peer IDs.
 *
 * @param[inout] p_peers   The buffer where to store the list of peer IDs.
 * @param[inout] p_size    In: The size of the @p p_peers buffer.
 *                         Out: The number of peers copied in the buffer.
 */
static void peer_list_get(pm_peer_id_t * p_peers, uint32_t * p_size)
{
	pm_peer_id_t peer_id;
	uint32_t     peers_to_copy;

	peers_to_copy = (*p_size < BLE_GAP_WHITELIST_ADDR_MAX_COUNT) ?
	                 *p_size : BLE_GAP_WHITELIST_ADDR_MAX_COUNT;

	peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
	*p_size = 0;

	while ((peer_id != PM_PEER_ID_INVALID) && (peers_to_copy--))
	{
		p_peers[(*p_size)++] = peer_id;
		peer_id = pm_next_peer_id_get(peer_id);
	}
}
/**@brief Function for starting advertising.
 */
static void advertising_start(bool erase_bonds)
{
	if (erase_bonds == true)
	{
		delete_bonds();
		// Advertising is started by PM_EVT_PEERS_DELETE_SUCCEEDED event.
	}
	else
	{
		ret_code_t ret;

		memset(m_whitelist_peers, PM_PEER_ID_INVALID, sizeof(m_whitelist_peers));
		m_whitelist_peer_cnt = (sizeof(m_whitelist_peers) / sizeof(pm_peer_id_t));

		peer_list_get(m_whitelist_peers, &m_whitelist_peer_cnt);

		ret = pm_whitelist_set(m_whitelist_peers, m_whitelist_peer_cnt);
		APP_ERROR_CHECK(ret);

		// Setup the device identies list.
		// Some SoftDevices do not support this feature.
		ret = pm_device_identities_list_set(m_whitelist_peers, m_whitelist_peer_cnt);
		if (ret != NRF_ERROR_NOT_SUPPORTED)
		{
			APP_ERROR_CHECK(ret);
		}

		ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
		APP_ERROR_CHECK(ret);
	}
}

const struct nrf_advertising NRF_Advertising = { 
	.advertising_init = advertising_init,
	.peer_list_get = peer_list_get,
	.advertising_start = advertising_start,
	.get_m_whitelist_peer_cnt = get_m_whitelist_peer_cnt,
	.add_m_whitelist_peer = add_m_whitelist_peer,
	.get_m_whitelist_peers = get_m_whitelist_peers
};
