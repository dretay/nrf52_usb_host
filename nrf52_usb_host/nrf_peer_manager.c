#include "nrf_peer_manager.h"


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
	ret_code_t err_code;
	pm_peer_id_t      m_peer_id;

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

			m_peer_id = p_evt->peer_id;
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
			pm_conn_sec_config_t conn_sec_config = { .allow_repairing = false };
			pm_conn_sec_config_reply(p_evt->conn_handle, &conn_sec_config);
		} break;

	case PM_EVT_STORAGE_FULL:
		{
			// Run garbage collection on the flash.
			err_code = fds_gc();
			if (err_code == FDS_ERR_NO_SPACE_IN_QUEUES)
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
			NRF_Advertising.advertising_start(false);
		} break;

	case PM_EVT_PEER_DATA_UPDATE_SUCCEEDED:
		{
			if (p_evt->params.peer_data_update_succeeded.flash_changed
			     && (p_evt->params.peer_data_update_succeeded.data_id == PM_PEER_DATA_ID_BONDING))
			{
				NRF_LOG_INFO("New Bond, add the peer to the whitelist if possible");
				NRF_LOG_INFO("\tm_whitelist_peer_cnt %d, MAX_PEERS_WLIST %d",
					NRF_Advertising.get_m_whitelist_peer_cnt() + 1,
					BLE_GAP_WHITELIST_ADDR_MAX_COUNT);
				// Note: You should check on what kind of white list policy your application should use.

				if(NRF_Advertising.get_m_whitelist_peer_cnt() < BLE_GAP_WHITELIST_ADDR_MAX_COUNT)
				{
					// Bonded to a new peer, add it to the whitelist.
					NRF_Advertising.add_m_whitelist_peer(&m_peer_id);

					// The whitelist has been modified, update it in the Peer Manager.
					err_code = pm_whitelist_set(NRF_Advertising.get_m_whitelist_peers(), NRF_Advertising.get_m_whitelist_peer_cnt());
					APP_ERROR_CHECK(err_code);

					err_code = pm_device_identities_list_set(NRF_Advertising.get_m_whitelist_peers(), NRF_Advertising.get_m_whitelist_peer_cnt());
					if (err_code != NRF_ERROR_NOT_SUPPORTED)
					{
						APP_ERROR_CHECK(err_code);
					}
				}
			}
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
	case PM_EVT_PEER_DELETE_SUCCEEDED:
	case PM_EVT_LOCAL_DB_CACHE_APPLIED:
	case PM_EVT_LOCAL_DB_CACHE_APPLY_FAILED:
		// This can happen when the local DB has changed.
	case PM_EVT_SERVICE_CHANGED_IND_SENT :
	case PM_EVT_SERVICE_CHANGED_IND_CONFIRMED :
	default :
	    break;
	}
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
	sec_param.lesc           = SEC_PARAM_LESC;
	sec_param.keypress       = SEC_PARAM_KEYPRESS;
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

const struct nrf_peer_manager NRF_Peer_Manager = { 
	.peer_manager_init = peer_manager_init,
	.pm_evt_handler = pm_evt_handler
	
};
