/*
 * Simple echo-repeater code example for Attiny13
 * Receives a 2-byte command and retransmits it after a 2-second delay
 *
 * Author : SinuX
 */

#define F_CPU				1200000UL	// Clock speed 1.2 MHz (-Ulfuse:w:0x6A:m)
#define TRF_RX_PIN          PB0			// Receiver on Pin 0
#define TRF_TX_PIN          PB1			// Transmitter on Pin 1
#define TRF_DATA_SIZE       2			// Command size 2 bytes
#define TRF_TX_REPEAT_COUNT 20			// Send received message 20 times

#include "tinyrf.h"
#include <util/delay.h>

int main(void) {
    while (1) {
		// Initialize timer and ports
		trf_init();
		
		// If a new command is available
		if (trf_has_received_data()) {
			// Get received data
			uint8_t data_buffer[TRF_DATA_SIZE];
			trf_get_received_data(data_buffer);

			// Check data integrity (second byte must have inverted value of the first)
			if (data_buffer[0] == (uint8_t)(~data_buffer[1])) {
				// Transmit received command after 2 seconds delay
				_delay_ms(2000);
				trf_send(data_buffer);
			}

			// Reset state to listen for new messages
			trf_reset_received();
		}
    }
}

