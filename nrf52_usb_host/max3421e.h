#pragma once
/*
 * MAX3421E.h
 *
 *  Created on: 24 Aug 2016
 *      Author: Stefan van der Linden
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "simple_spi.h"
#include "usb.h"
#include "packets.h"

#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "nrf_spi_mngr.h"


/*** MACROs ***/

#define DELAY_WITH_TIMEOUT(STATEMENT)   uint_fast8_t __it__ = 0; \
                                        while(__it__ < SPI_TIMEOUT && STATEMENT) { __it__++; }

/* DEFINES */
#define MAX_IRQ_PIN 30
#define BIT0                   (0x0001)
#define BIT1                   (0x0002)
#define BIT2                   (0x0004)
#define BIT3                   (0x0008)
#define BIT4                   (0x0010)
#define BIT5                   (0x0020)
#define BIT6                   (0x0040)
#define BIT7                   (0x0080)
#define BIT8                   (0x0100)
#define BIT9                   (0x0200)
#define BITA                   (0x0400)
#define BITB                   (0x0800)
#define BITC                   (0x1000)
#define BITD                   (0x2000)
#define BITE                   (0x4000)
#define BITF                   (0x8000)


#define DIR_WRITE 1
#define DIR_READ  0

#define BUFFER_SIZE 64

#define MAX_IRQ_BUSEVENT    BIT0
#define MAX_IRQ_RWU         BIT1
#define MAX_IRQ_RCVDAV      BIT2
#define MAX_IRQ_SNDBAV      BIT3
#define MAX_IRQ_SUSDN       BIT4
#define MAX_IRQ_CONDET      BIT5
#define MAX_IRQ_FRAME       BIT6
#define MAX_IRQ_HXFRDN      BIT7

#define MAX_IRQ_OSCOK       BIT0
/* MAX_IRQ_RWU */
#define MAX_IRQ_BUSACT      BIT2
#define MAX_IRQ_URES        BIT3
#define MAX_IRQ_SUSP        BIT4
#define MAX_IRQ_NOVBUS      BIT5
#define MAX_IRQ_VBUS        BIT6
#define MAX_IRQ_URESDN      BIT7

/* the End Point interrupts (EPIRQ) */
#define MAX_IRQ_IN0BAV      BIT0
#define MAX_IRQ_OUT0DAV     BIT1
#define MAX_IRQ_OUT1DAV     BIT2
#define MAX_IRQ_IN2BAV      BIT3
#define MAX_IRQ_IN3BAV      BIT4
#define MAX_IRQ_SUDAV       BIT5


#define MODE_PERIPH         0
#define MODE_HOST           1

#define rEP0FIFO        0
#define rEP1OUTFIFO     1
#define rEP2INFIFO      2
#define rEP3INFIFO      3
#define rSUDFIFO        4
#define rEP0BC          5
#define rEP1OUTBC       6
#define rEP2INBC        7
#define rEP3INBC        8
#define rEPSTALLS       9
#define rCLRTOGS        10
#define rEPIRQ          11
#define rEPIEN          12
#define rUSBIRQ         13
#define rUSBIEN         14
#define rUSBCTL         15
#define rCPUCTL         16
#define rPINCTL         17
#define rREVISION       18
#define rFNADDR         19
#define rIOPINS         20

#define rRCVFIFO        1
#define rSNDFIFO        2
#define rSUDFIFO        4
#define rRCVBC          6
#define rSNDBC          7
#define rIOPINS1        20
#define rIOPINS2        21
#define rGPINIRQ        22
#define rGPINIEN        23
#define rGPINPOL        24
#define rHIRQ           25
#define rHIEN           26
#define rMODE           27
#define rPERADDR        28
#define rHCTL           29
#define rHXFR           30
#define rHRSL           31


/* Change this GPIO pin to change the interrupt pin */
#define USBINT_PORT GPIO_PORT_P2
#define USBINT_PIN  GPIO_PIN3

/* PROTOTYPES */

/**
 * Start the module and initialise
 *
 * Parameters:
 * uint_fast8_t startAsMaster: set to true if the module should be initialised as a USB master
 */
void MAX_start(uint_fast8_t);

/**
 * Reset the module by enabling CHIPRES
 */
void MAX_reset(void);

/**
 * Write to the specified register
 *
 * Parameters:
 * uint_fast8_t address: register address to write to
 * uint_fast8_t value: the value to write to the specified register
 *
 * Returns:
 * uint_fast8_t: the byte read concurrently on MISO while writing on MOSI
 */
uint_fast8_t MAX_writeRegister(uint_fast8_t, uint_fast8_t);

/**
 * Write to the specified register, including the ACKSTAT bit set to true
 *
 * Parameters:
 * uint_fast8_t address: register address to write to
 * uint_fast8_t value: the value to write to the specified register
 *
 * Returns:
 * uint_fast8_t: the byte read concurrently on MISO while writing on MOSI
 */
uint_fast8_t MAX_writeRegisterAS(uint_fast8_t, uint_fast8_t);

/**
 * Write to the specified register
 *
 * Parameters:
 * uint_fast8_t address: register address to write to
 * uint_fast8_t * values: an array of values to write to the specified register
 * uint_fast8_t length: the length of the 'values' array
 *
 * Returns:
 * uint_fast8_t: the byte read concurrently on MISO while writing the last byte on MOSI
 */
uint_fast8_t MAX_multiWriteRegister(uint_fast8_t, uint_fast8_t *, uint_fast8_t);

/**
 * Read multiple bytes from a register
 *
 * Parameters:
 * uint_fast8_t address: register address to read from
 * uint_fast8_t * buffer: an array of values to store the result in
 * uint_fast8_t length: the number of bytes to read
 *
 */
void MAX_multiReadRegister(uint_fast8_t, uint_fast8_t *, uint_fast8_t);

/**
 * Read multiple bytes from a register, setting ACKSTAT to true
 *
 * Parameters:
 * uint_fast8_t address: register address to read from
 * uint_fast8_t * buffer: an array of values to store the result in
 * uint_fast8_t length: the number of bytes to read
 *
 */
uint_fast8_t MAX_readRegisterAS(uint_fast8_t);

/**
 * Read from the specified register
 *
 * Parameters:
 * uint_fast8_t address: register address to read from
 *
 * Returns:
 * uint_fast8_t: the byte read
 */
uint_fast8_t MAX_readRegister(uint_fast8_t);

/**
 * Enable the specified bits
 *
 * Parameters:
 * uint_fast8_t address: register address to enable the bits in
 * uint_fast8_t flags: the bits to enable
 */
void MAX_enableOptions(uint_fast8_t, uint_fast8_t);

/**
 * Disable the specified bits
 *
 * Parameters:
 * uint_fast8_t address: register address to disable the bits in
 * uint_fast8_t flags: the bits to disable
 */
void MAX_disableOptions(uint_fast8_t, uint_fast8_t);



/**
 * Enable interrupts
 *
 * Parameters:
 * uint_fast8_t flags: the interrupts to enable
 */
void MAX_enableInterrupts(uint_fast8_t);

/**
 * Disable interrupts
 *
 * Parameters:
 * uint_fast8_t flags: the interrupts to disable
 */
void MAX_disableInterrupts(uint_fast8_t);

/**
 * Enable EP interrupts
 *
 * Parameters:
 * uint_fast8_t flags: the EP interrupts to enable
 */
void MAX_enableEPInterrupts(uint_fast8_t);

/**
 * Disable EP interrupts
 *
 * Parameters:
 * uint_fast8_t flags: the EP interrupts to disable
 */
void MAX_disableEPInterrupts(uint_fast8_t);

/**
 * Clear the interrupt statuses
 *
 * Parameters:
 * uint_fast8_t flags: the interrupts to clear
 */
void MAX_clearInterruptStatus(uint_fast8_t);

/**
 * Clear the EP interrupt statuses
 *
 * Parameters:
 * uint_fast8_t flags: the EP interrupts to clear
 */
void MAX_clearEPInterruptStatus(uint_fast8_t);

/**
 * Enable the interrupt pin
 */
void MAX_enableInterruptsMaster(void);

/**
 * Disable the interrupt pin
 */
void MAX_disableInterruptsMaster(void);

/**
 * Get the status of the interrupts
 *
 * Returns:
 * uint_fast8_t: a byte containing the status of the interrupts
 */
uint_fast8_t MAX_getInterruptStatus(void);

/**
 * Get the status of enabled interrupts
 *
 * Returns:
 * uint_fast8_t: a byte containing the status of the enabled interrupts
 */
uint_fast8_t MAX_getEnabledInterruptStatus(void);

/**
 * Get the status of the EP interrupts
 *
 * Returns:
 * uint_fast8_t: a byte containing the status of the EP interrupts
 */
uint_fast8_t MAX_getEPInterruptStatus(void);

/**
 * Get the status of enabled EP interrupts
 *
 * Returns:
 * uint_fast8_t: a byte containing the status of the enabled EP interrupts
 */
uint_fast8_t MAX_getEnabledEPInterruptStatus(void);

/**
 * Set a function to be called when a peripheral connects or disconnects
 *
 * Parameters: the function pointer to be set
 */
void MAX_setStateChangeIRQ(void(*handle)(uint_fast8_t));

/**
 * Scan the bus' state
 *
 * Returns:
 * uint_fast8_t: the J/K state: 0x0 = SE0, 0x1 = K, 0x2 = J, 0x3 = N/A
 */
uint_fast8_t MAX_scanBus(void);

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
