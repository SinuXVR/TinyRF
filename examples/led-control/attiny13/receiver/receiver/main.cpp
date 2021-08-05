/*
 * Simple receiver code example for Attiny13
 * Takes 8 commands, does some data integrity check, and turns 4 LEDs on/off
 *
 * Author : SinuX
 */ 

#define F_CPU				1200000UL	// Clock speed 1.2 MHz (-Ulfuse:w:0x6A:m)
#define TRF_RX_PIN			PB0			// Receiver on Pin 0
#define TRF_DATA_SIZE		2			// Command size 2 bytes
#define TRF_TX_DISABLED					// Exclude transmitter code to preserve space

#include "tinyrf.h"

// Pin definitions for 4 LEDs
#define LED_1_PIN			PB3
#define LED_2_PIN			PB4
#define LED_3_PIN			PB2
#define LED_4_PIN			PB1

// LED pins control macro
#define init_led_pins() DDRB |= (1 << LED_2_PIN) | (1 << LED_1_PIN) | (1 << LED_3_PIN) | (1 << LED_4_PIN)
#define set_led_pin(pin, state) state ? PORTB |= (1 << pin) : PORTB &= ~(1 << pin)

int main(void)
{
	// Initialize timer and ports
	trf_init();
	init_led_pins();
	
	while (1)
	{
		// If a new command is available
		if (trf_has_received_data()) {
			
			// Get received data
			uint8_t data_buffer[TRF_DATA_SIZE];
			trf_get_received_data(data_buffer);
			
			// Check data integrity (second byte must have inverted value of the first)
			if (data_buffer[0] == (uint8_t)(~data_buffer[1])) {
				switch (data_buffer[0]) {
					case 11:
					set_led_pin(LED_1_PIN, 1);	// Turn on LED1
					break;
					case 22:
					set_led_pin(LED_2_PIN, 1);	// Turn on LED2
					break;
					case 33:
					set_led_pin(LED_3_PIN, 1);	// Turn on LED3
					break;
					case 44:
					set_led_pin(LED_4_PIN, 1);	// Turn on LED4
					break;
					case 55:
					set_led_pin(LED_1_PIN, 0);	// Turn off LED1
					break;
					case 66:
					set_led_pin(LED_2_PIN, 0);	// Turn off LED2
					break;
					case 77:
					set_led_pin(LED_3_PIN, 0);	// Turn off LED3
					break;
					case 88:
					set_led_pin(LED_4_PIN, 0);	// Turn off LED4
					break;
					default:
					break;
				}
			}
			
			// Reset state to listen for new messages
			trf_reset_received();
		}
	}
}
