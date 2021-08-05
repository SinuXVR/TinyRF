/*
 * Simple transmitter code example for Attiny13
 * Sends 8 different commands sequentially, 2 bytes each 
 *
 * Author : SinuX
 */ 

#define F_CPU				1200000UL	// Clock speed 1.2 MHz (-Ulfuse:w:0x6A:m)
#define TRF_TX_PIN			PB0			// Transmitter on Pin 0
#define TRF_DATA_SIZE		2			// Command size 2 bytes
#define TRF_RX_DISABLED					// Exclude receiver code to preserve space

#include "tinyrf.h"
#include <util/delay.h>

// Second byte of the command has the inverted value of the first to perform simple data integrity control on the receiver
uint8_t commands[8][TRF_DATA_SIZE] = {
	{ 11, (uint8_t) ~11 },
	{ 22, (uint8_t) ~22 },
	{ 33, (uint8_t) ~33 },
	{ 44, (uint8_t) ~44 },
	{ 55, (uint8_t) ~55 },
	{ 66, (uint8_t) ~66 },
	{ 77, (uint8_t) ~77 },
	{ 88, (uint8_t) ~88 }
};

int main(void) {
	// Initialize timer and ports
	trf_init();
	
    while (1) {
		// Send commands with 0.5 sec interval
		for (uint8_t i = 0; i < 8; i++) {
			trf_send(commands[i]);
			_delay_ms(500);
		}
    }
}
