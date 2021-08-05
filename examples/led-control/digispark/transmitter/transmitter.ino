/*
 * Simple transmitter sketch example for Digispark (Attiny85)
 * Sends 8 different commands sequentially, 2 bytes each 
 *
 * Author : SinuX
 */ 

#define TRF_TX_PIN PB1  // Transmitter on Pin 1
#define TRF_DATA_SIZE 2 // Command size 2 bytes
#define TRF_RX_DISABLED // Exclude receiver code to preserve space

#include "tinyrf.h"
#include <util/delay.h>

// Second byte of the command has the inverted value of the first to perform simple data integrity control on the receiver
byte commands[8][TRF_DATA_SIZE] = {
  { 11, (byte) ~11 },
  { 22, (byte) ~22 },
  { 33, (byte) ~33 },
  { 44, (byte) ~44 },
  { 55, (byte) ~55 },
  { 66, (byte) ~66 },
  { 77, (byte) ~77 },
  { 88, (byte) ~88 }
};

void setup() {
  // Initialize timer and ports
  trf_init();
}

void loop() {
  // Send commands with 0.5 sec interval
  for (byte i = 0; i < 8; i++) {
      trf_send(commands[i]);
      _delay_ms(500);
    }
}
