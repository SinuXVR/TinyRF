/*
 * Simple receiver code example for Digispark (Attiny85)
 * Takes 8 commands, does some data integrity check, and turns embedded LED on/off
 *
 * Author : SinuX
 */ 

#define TRF_RX_PIN PB0  // Receiver on Pin 0
#define TRF_DATA_SIZE 2 // Command size 2 bytes
#define TRF_TX_DISABLED // Exclude transmitter code to preserve space

#include "tinyrf.h"

#define LED_PIN PB1 // Embedded LED pin (PB1 for Digispark)

void setup() {
  // Initialize timer and ports
  trf_init();
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // If a new command is available
  if (trf_has_received_data()) {
    
    // Get received data
    byte data[TRF_DATA_SIZE];
    trf_get_received_data(data);

    // Check data integrity (second byte must have inverted value of the first)
    if (data[0] == (byte)(~data[1])) {
      switch(data[0]) {
        case 11:
        case 33:
        case 55:
        case 77:
          digitalWrite(LED_PIN, HIGH);  // Turn on LED
          break;
        case 22:
        case 44:
        case 66:
        case 88:
          digitalWrite(LED_PIN, LOW);   // Turn off LED
          break;
        default:
          break;
      }
    }
    
    // Reset state to listen for new messages
    trf_reset_received();
  }
}
