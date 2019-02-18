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
#include "ble_hids.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "ble_conn_params.h"
#include "sensorsim.h"
#include "bsp_btn_ble.h"
#include "app_scheduler.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "ble_advertising.h"
#include "fds.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_battery.h"
#include "nrf_advertising.h"
#include "nrf_gap.h"
#include "nrf_peer_manager.h"
#include "nrf_services.h"
#include "nrf_ble_stack.h"
#include "nrf_connection.h"
#include "nrf_bsp.h"

#include "max3421e.h"

#include "nrf_spi_mngr.h"

#include "app_uart.h"
#include "app_error.h"


uint16_t          m_conn_handle  = BLE_CONN_HANDLE_INVALID; /**< Handle of the current connection. */
NRF_BLE_QWR_DEF(m_qwr); /**< Context for the Queued Write module.*/


#define SCHED_MAX_EVENT_DATA_SIZE       APP_TIMER_SCHED_EVENT_DATA_SIZE             /**< Maximum size of scheduler events. */
#ifdef SVCALL_AS_NORMAL_FUNCTION
#define SCHED_QUEUE_SIZE                20                                          /**< Maximum number of events in the scheduler queue. More is needed in case of Serialization. */
#else
#define SCHED_QUEUE_SIZE                10                                          /**< Maximum number of events in the scheduler queue. */
#endif

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

APP_TIMER_DEF(m_battery_timer_id);                                                  /**< Battery timer. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */


static void on_hids_evt(ble_hids_t * p_hids, ble_hids_evt_t * p_evt);


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    ret_code_t err_code;

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Create battery timer.
    err_code = app_timer_create(&m_battery_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                NRF_Battery.battery_level_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting timers.
 */
static void timers_start(void)
{
    ret_code_t err_code;

    err_code = app_timer_start(m_battery_timer_id, BATTERY_LEVEL_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    app_sched_execute();
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

 
#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 1
 
void uart_error_handle(app_uart_evt_t * p_event)
{
	if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
	{
		APP_ERROR_HANDLER(p_event->data.error_communication);
	}
	else if (p_event->evt_type == APP_UART_FIFO_ERROR)
	{
		APP_ERROR_HANDLER(p_event->data.error_code);
	}
}

volatile bool peripheralAvailable;
volatile uint_fast8_t RXData[BUFFER_SIZE];
void busStateChanged(uint_fast8_t newState) {
	uint_fast8_t result = MAX_scanBus();
	if (result == 0x01 || result == 0x02)
		peripheralAvailable = true;
	else
		peripheralAvailable = false;
}
/**@brief Function for application main entry.
 */
int main(void)
{   

	bool erase_bonds;

    // Initialize.
    log_init();
    timers_init();
    NRF_Bsp.buttons_leds_init(&erase_bonds);
    power_management_init();
	NRF_BLE_Stack.ble_stack_init();
    scheduler_init();
    NRF_Gap.gap_params_init();
    gatt_init();
    NRF_Advertising.advertising_init();
    NRF_Services.services_init();
    NRF_Battery.sensor_simulator_init();
    NRF_Connection.conn_params_init();
	NRF_Peer_Manager.peer_manager_init();

    // Start execution.
    timers_start();
	NRF_Advertising.advertising_start(erase_bonds);
	
	MAX_start(true);
	MAX_setStateChangeIRQ(&busStateChanged);

	/* Enable interrupts */
	MAX_enableInterrupts(MAX_IRQ_CONDET);
	MAX_clearInterruptStatus(MAX_IRQ_CONDET);
	MAX_enableInterruptsMaster();

	uint_fast8_t regval = MAX_readRegister(rREVISION);
	NRF_LOG_INFO("Revision: 0x%x\n", regval);


	/* Perform a bus reset to reconnect after a power down */
	if (!peripheralAvailable)
	{
		NRF_LOG_INFO("Resetting bus\n");
		USB_busReset();		
	}


    // Enter main loop.
    for (;;)
    {
	    if (peripheralAvailable) {
		    NRF_LOG_INFO("SUCCESS!!!!!!!!!");

		    /* Make sure the RX buffer is free */
		    MAX_writeRegister(rHIRQ, BIT2);

		    uint_fast8_t i, result;
		    uint_fast32_t totalRcvd = 0;

		    printf("Requesting data...\n");
		    printf("(Address: %d)\n", MAX_readRegister(rPERADDR));
		    MAX_writeRegister(rHIRQ, BIT2);
		    for (i = 0; i < 1000; i++) {
			    result = requestData((uint_fast8_t *) RXData, 64);
			    if (result != 0) {
				    printf("Result error: 0x%x (%d)\n", result, i);
			    }
			    else {
				    totalRcvd += 64;
			    }
		    }
		    printf("Received %d bytes!\n", totalRcvd);
	    }
		idle_state_handle();
    }
}


/**
 * @}
 */
