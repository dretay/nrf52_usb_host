#pragma once
/*
 * simple_spi.h
 *
 *  Created on: 27 Jun 2016
 *      Author: Stefan van der Linden
 */

/*** Defines ***/

#include <stdint.h>
#include "nrf_spi_mngr.h"
#include "nordic_common.h"


#define SPI_SCK_PIN 3
#define SPI_MISO_PIN 28
#define SPI_MOSI_PIN 4
#define SPI_SS_PIN 29
#define RESET_PIN 31
#define ST7565_QUEUE_LENGTH     15
#define ST7565_SPI_INSTANCE_ID  0
#define SPI_TIMEOUT         2400000

/*** MACROs ***/

#define DELAY_WITH_TIMEOUT(STATEMENT)   uint_fast8_t __it__ = 0; \
                                        while(__it__ < SPI_TIMEOUT && STATEMENT) { __it__++; }


/*** PROTOTYPES ***/

/**
 * Start the SPI module
 */
void SIMSPI_startSPI(void);

/**
 * Transmit and receive a single byte
 *
 * Parameters:
 * uint_fast8_t byte: the single byte to send
 *
 * Returns:
 * uint_fast8_t: the single byte received during the transmission
 */
uint_fast8_t SIMSPI_transmitByte(uint_fast8_t, uint_fast8_t);

/**
 * Transmit and receive an array of bytes
 *
 * Parameters:
 * uint_fast8_t bytes: the byte array to transmit
 * uint_fast8_t length: the amount of bytes in the given array
 *
 * Returns:
 * uint_fast8_t: the single byte received during the transmission
 */
uint_fast8_t SIMSPI_transmitBytes(uint_fast8_t *, uint_fast8_t);

/**
 * Transmit the given bytes and return all the received bytes
 *
 * Parameters:
 * uint_fast8_t * rxbuffer: a buffer able to hold the received bytes
 * uint_fast8_t * bytes: the byte array to transmit
 * uint_fast8_t length: the amount of bytes in the given array
 *
 * Returns:
 * uint_fast8_t *: a (dynamically allocated) pointer to the received bytes
 */
uint_fast8_t SIMSPI_transmitBytesReadAll(uint_fast8_t *, uint_fast8_t *, uint_fast8_t);

uint_fast8_t SIMSPI_readBytes(uint_fast8_t *, uint_fast8_t);
