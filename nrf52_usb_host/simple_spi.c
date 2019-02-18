/*
 * simple_spi.c
 *
 *  Created on: 27 Jun 2016
 *      Author: Stefan van der Linden
 */

#include "simple_spi.h"

NRF_SPI_MNGR_DEF(m_nrf_spi_mngr, ST7565_QUEUE_LENGTH, ST7565_SPI_INSTANCE_ID);
extern nrf_drv_spi_t spi;

void SIMSPI_startSPI(void) {
	nrf_drv_spi_config_t const m_master0_config =
	{
		.sck_pin = SPI_SCK_PIN,
		.mosi_pin = SPI_MOSI_PIN,
		.miso_pin = SPI_MISO_PIN,
		.ss_pin = SPI_SS_PIN,
		.irq_priority = APP_IRQ_PRIORITY_LOWEST,
		.orc = 0x00,
		.frequency = SPI_FREQUENCY_FREQUENCY_M4,
		.mode = NRF_DRV_SPI_MODE_0,
		.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST
	};
	nrf_spi_mngr_init(&m_nrf_spi_mngr, &m_master0_config);
}

uint_fast8_t SIMSPI_transmitByte(uint_fast8_t byte1, uint_fast8_t byte2) {
	uint_fast8_t rx[2];
	uint8_t tx[] = { (uint8_t)byte1, (uint8_t)byte2 };
	// MAX_CS_PORT->BSRR = (uint32_t)MAX_CS_PIN << 16;
	nrf_spi_mngr_transfer_t const transfers[] =
	{
		NRF_SPI_MNGR_TRANSFER(tx, 2, rx, 2),		
	};
	nrf_spi_mngr_perform(&m_nrf_spi_mngr, NULL, transfers, ARRAY_SIZE(transfers), NULL);

	return rx[1];
}

uint_fast8_t SIMSPI_transmitBytes(uint_fast8_t * bytes, uint_fast8_t length) {
	uint_fast8_t it;
	
	for (it = 0; it < length - 1; it++) {
		/* Transmit the current byte */
		SIMSPI_transmitByte(bytes[it],0);
	}
	return SIMSPI_transmitByte(bytes[it++],0);
}

uint_fast8_t SIMSPI_transmitBytesReadAll( uint_fast8_t * rxbuffer,
	uint_fast8_t * bytes,
	uint_fast8_t length) {
	uint_fast8_t it;
	
	for (it = 0; it < length; it++) {
		/* Transmit the current byte */
		rxbuffer[it] = SIMSPI_transmitByte(bytes[it],0);
	}
	return 0;
}

uint_fast8_t SIMSPI_readBytes(uint_fast8_t * rxbuffer, uint_fast8_t length) {
	uint_fast8_t it;

	for (it = 0; it < length; it++) {
		/* Transmit the current byte */
		rxbuffer[it] = SIMSPI_transmitByte(0,0);
	}
	return 0;
}