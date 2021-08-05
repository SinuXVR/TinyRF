/*
 * TinyRF - simple and lightweight library for remote control or
 * data exchange between ATtiny13, ATtiny85 and some other ATtiny MCUs.
 *
 * Copyright (c) 2021, SinuX. All rights reserved.
 * 
 * This library is distributed "as is" without any warranty.
 * See MIT License for more details.
 */

#ifndef TINYRF_H_
#define TINYRF_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef F_CPU
# error "F_CPU is required for <tinyrf.h>. Please, provide actual clock speed in Hz"
#endif

/************************************************************************/
/* Common                                                               */
/************************************************************************/

// Data size in bytes
#ifndef TRF_DATA_SIZE
# define TRF_DATA_SIZE		3
#elif TRF_DATA_SIZE < 1 || TRF_DATA_SIZE > 255
# error "TRF_DATA_SIZE must be in range 1 - 255"
#endif

// Single pulse width in microseconds, default value is 350us
#ifndef TRF_PULSE_WIDTH
# define TRF_PULSE_WIDTH	350
#endif

// Sync high and low pulses count, default values are {1; 31}:
//  _
// |1|_______________________________ 31
//
#ifndef TRF_SYNC_HIGH_PULSES
# define TRF_SYNC_HIGH_PULSES	1
#elif TRF_SYNC_HIGH_PULSES < 0
# error "TRF_SYNC_HIGH_PULSES must be > 0"
#endif
#ifndef TRF_SYNC_LOW_PULSES
# define TRF_SYNC_LOW_PULSES	31
#elif TRF_SYNC_LOW_PULSES < 0
# error "TRF_SYNC_LOW_PULSES must be > 0"
#endif
#if TRF_SYNC_HIGH_PULSES + TRF_SYNC_LOW_PULSES > 255
# error "TRF_SYNC_HIGH_PULSES + TRF_SYNC_LOW_PULSES must be <= 255"
#endif

// "1" high and low pulses count, default values are {3; 1}:
//  ___
// | 3 |_ 1
//
#ifndef TRF_ONE_HIGH_PULSES
# define TRF_ONE_HIGH_PULSES	3
#elif TRF_ONE_HIGH_PULSES < 0
# error "TRF_ONE_HIGH_PULSES must be > 0"
#endif
#ifndef TRF_ONE_LOW_PULSES
# define TRF_ONE_LOW_PULSES		1
#elif TRF_ONE_LOW_PULSES < 0
# error "TRF_ONE_LOW_PULSES must be > 0"
#endif
#if TRF_ONE_HIGH_PULSES + TRF_ONE_LOW_PULSES > 255
# error "TRF_ONE_HIGH_PULSES + TRF_ONE_LOW_PULSES must be <= 255"
#endif

// "0" high and low pulses count, default values are {1; 3}:
//  _
// |1|___ 3
//
#ifndef TRF_ZERO_HIGH_PULSES
# define TRF_ZERO_HIGH_PULSES	1
#elif TRF_ZERO_HIGH_PULSES < 0
# error "TRF_ZERO_HIGH_PULSES must be > 0"
#endif
#ifndef TRF_ZERO_LOW_PULSES
# define TRF_ZERO_LOW_PULSES	3
#elif TRF_ZERO_LOW_PULSES < 0
# error "TRF_ZERO_LOW_PULSES must be > 0"
#endif
#if TRF_ZERO_HIGH_PULSES + TRF_ZERO_LOW_PULSES > 255
# error "TRF_ZERO_HIGH_PULSES + TRF_ZERO_LOW_PULSES must be <= 255"
#endif

/*
 * Attention: the protocol implementation is different from the classic EV1527
 * It sends the sync sequence twice (at the beginning of each packet and at the end of the entire transmission)
 * Classic EV1527 sends a sync sequence at the end of each packet
 * But in general cases (when TRF_TX_REPEAT_COUNT> 2 they are compatible)
 */

#define _trf_throw_compile_time_error(msg) ({ extern uint8_t __attribute__((error(#msg))) compile_time_check(); compile_time_check(); })

// Use OCR0B instead of volatile variable to store timer overflow count
#define _trf_timer_ovf_count OCR0B

static inline void _trf_init_timer() {
	// Calculate real prescaler value
	const double timer_prescaler_real = (double) F_CPU * TRF_PULSE_WIDTH / 1000000.0d / 255.0d;
	uint8_t timer_prescaler;
	uint16_t timer_counter_max;
	
	// Calculate prescaler and counter max value
	if (timer_prescaler_real <= 1) {
		timer_prescaler = 1 << CS00;					// CLK/1
		timer_counter_max = (uint16_t) ((double) F_CPU * TRF_PULSE_WIDTH / 1000000.0d / 1.0d) - 1;
	} else if (timer_prescaler_real <= 8) {
		timer_prescaler = 1 << CS01;					// CLK/8
		timer_counter_max = (uint16_t) ((double) F_CPU * TRF_PULSE_WIDTH / 1000000.0d / 8.0d) - 1;
	} else if (timer_prescaler_real <= 64) {
		timer_prescaler = (1 << CS01) | (1 << CS00);	// CLK/64
		timer_counter_max = (uint16_t) ((double) F_CPU * TRF_PULSE_WIDTH / 1000000.0d / 64.0d) - 1;
	} else if (timer_prescaler_real <= 256) {
		timer_prescaler = 1 << CS02;					// CLK/256
		timer_counter_max = (uint16_t) ((double) F_CPU * TRF_PULSE_WIDTH / 1000000.0d / 256.0d) - 1;
	} else if (timer_prescaler_real <= 1024) {
		timer_prescaler = (1 << CS02) | (1 << CS00);	// CLK/1024
		timer_counter_max = (uint16_t) ((double) F_CPU * TRF_PULSE_WIDTH / 1000000.0d / 1024.0d) - 1;
	} else {
		_trf_throw_compile_time_error("Couldn't find any valid timer prescaler value to handle pulse duration. Please, adjust TRF_PULSE_WIDTH or F_CPU value.");
	}
	
	// Validate timer counter max value
	if (timer_counter_max < 1 || timer_counter_max > 255) {
		_trf_throw_compile_time_error("Timer counter max value is out of range. Please, adjust TRF_PULSE_WIDTH or F_CPU value.");
	}
	
	// Setup timer (CTC mode, enable interrupt)
	TCCR0A = 1 << WGM01;
	TCCR0B = timer_prescaler;
	OCR0A = (uint8_t) timer_counter_max;
	
	#if defined (__AVR_ATtiny13__) || defined (__AVR_ATtiny13A__) 
	TIMSK0 = 1 << OCIE0A;
	#else
	// Compatibility with some other ATtiny MCUs
	TIMSK = 1 << OCIE0A;
	#endif
}

static inline void _trf_reset_timer() {
	TCNT0 = 0;
	_trf_timer_ovf_count = 0;
}

ISR(TIM0_COMPA_vect) {
	_trf_timer_ovf_count++;
}

/************************************************************************/
/* Receiver                                                             */
/************************************************************************/

#ifndef TRF_RX_DISABLED

// Default RX pin is PB1
#ifndef TRF_RX_PIN
# define TRF_RX_PIN	PB1
#endif

// Set TRF_RX_PIN as input and attach PCINT
#define _trf_set_rx_pin_in() DDRB &= ~(1 << TRF_RX_PIN); GIMSK |= (1 << PCIE); PCMSK |= (1 << TRF_RX_PIN)

#define TRF_STATE_WAITING	0
#define TRF_STATE_RECEIVING	1
#define TRF_STATE_RECEIVED	2

static volatile uint8_t _trf_current_state = TRF_STATE_WAITING;
static volatile uint8_t _trf_received_data[TRF_DATA_SIZE];

// Check if a new message is available
static inline uint8_t trf_has_received_data() {
	return _trf_current_state == TRF_STATE_RECEIVED;
}

// Copy received message bytes to buffer
static inline void trf_get_received_data(uint8_t *buffer) {
	for (uint8_t i = 0; i < TRF_DATA_SIZE; i++) {
		buffer[i] = _trf_received_data[i];
	}
}

// Reset state to start searching for a new message
static inline void trf_reset_received() {
	_trf_current_state = TRF_STATE_WAITING;
}

static inline uint8_t _trf_is_in_range(uint8_t value, uint8_t min, uint8_t max) {
	return value >= min && value <= max;
}

ISR (PCINT0_vect) {
	// Ignore new message if previous one has not been read
	if (_trf_current_state == TRF_STATE_RECEIVED) return;
	
	// Determine changed pin and skip if it's not a TRF_RX_PIN
	static uint8_t prev_port_state = 0xFF;
	uint8_t changed_bits = PINB ^ prev_port_state;
	prev_port_state = PINB;
	if ((changed_bits & (1 << TRF_RX_PIN)) == 0) return;
	
	// Pulses count check ranges (pulses_count ±70%)
    const uint8_t sync_high_pulses_min = (uint8_t)(TRF_SYNC_HIGH_PULSES - (double) TRF_SYNC_HIGH_PULSES / 100.0d * 70.0d);
    const uint8_t sync_high_pulses_max = (uint8_t)(TRF_SYNC_HIGH_PULSES + (double) TRF_SYNC_HIGH_PULSES / 100.0d * 70.0d);
    const uint8_t sync_low_pulses_min = (uint8_t)(TRF_SYNC_LOW_PULSES - (double) TRF_SYNC_LOW_PULSES / 100.0d * 70.0d);
    const uint8_t sync_low_pulses_max = (uint8_t)(TRF_SYNC_LOW_PULSES + (double) TRF_SYNC_LOW_PULSES / 100.0d * 70.0d);
    const uint8_t one_high_pulses_min = (uint8_t)(TRF_ONE_HIGH_PULSES - (double) TRF_ONE_HIGH_PULSES / 100.0d * 70.0d);
    const uint8_t one_high_pulses_max = (uint8_t)(TRF_ONE_HIGH_PULSES + (double) TRF_ONE_HIGH_PULSES / 100.0d * 70.0d);
    const uint8_t one_low_pulses_min = (uint8_t)(TRF_ONE_LOW_PULSES - (double) TRF_ONE_LOW_PULSES / 100.0d * 70.0d);
    const uint8_t one_low_pulses_max = (uint8_t)(TRF_ONE_LOW_PULSES + (double) TRF_ONE_LOW_PULSES / 100.0d * 70.0d);
	const uint8_t zero_high_pulses_min = (uint8_t)(TRF_ZERO_HIGH_PULSES - (double) TRF_ZERO_HIGH_PULSES / 100.0d * 70.0d);
	const uint8_t zero_high_pulses_max = (uint8_t)(TRF_ZERO_HIGH_PULSES + (double) TRF_ZERO_HIGH_PULSES / 100.0d * 70.0d);
	const uint8_t zero_low_pulses_min = (uint8_t)(TRF_ZERO_LOW_PULSES - (double) TRF_ZERO_LOW_PULSES / 100.0d * 70.0d);
	const uint8_t zero_low_pulses_max = (uint8_t)(TRF_ZERO_LOW_PULSES + (double) TRF_ZERO_LOW_PULSES / 100.0d * 70.0d);
	
	uint8_t current_timer_ovf = _trf_timer_ovf_count;
	static uint8_t prev_timer_ovf = 0xFF;
	static uint8_t bit_counter = 0;
	static uint8_t byte_counter = 0;
	static uint8_t byte_buffer = 0;
	
	_trf_reset_timer();
	
	// Rising edge
	if (PINB & (1 << TRF_RX_PIN)) {
		// Check if SYNC sequence is just received
		if (_trf_current_state == TRF_STATE_WAITING) {
			if (_trf_is_in_range(prev_timer_ovf, sync_high_pulses_min, sync_high_pulses_max) &&
				_trf_is_in_range(current_timer_ovf, sync_low_pulses_min, sync_low_pulses_max)) {
				// Start data receiving
				_trf_current_state = TRF_STATE_RECEIVING;
				bit_counter = 0;
				byte_counter = 0;
			}
		} else {
			// _trf_current_state == TRF_STATE_RECEIVING - validate next received sequence
			if (_trf_is_in_range(prev_timer_ovf, one_high_pulses_min, one_high_pulses_max) &&
				_trf_is_in_range(current_timer_ovf, one_low_pulses_min, one_low_pulses_max)) {
					// ONE sequence detected - push 1 to buffer
					byte_buffer = (byte_buffer << 1) | 1;
					bit_counter++;
			} else if (_trf_is_in_range(prev_timer_ovf, zero_high_pulses_min, zero_high_pulses_max) &&
					  _trf_is_in_range(current_timer_ovf, zero_low_pulses_min, zero_low_pulses_max)) {
						  // ZERO sequence detected - push 0 to buffer
						  byte_buffer <<= 1;
						  bit_counter++;
			} else {
				// Cancel receiving if sequence is not valid
				_trf_current_state = TRF_STATE_WAITING;
			}
			
			// Push next received byte to data buffer
			if (bit_counter >= 8) {
				bit_counter = 0;
				_trf_received_data[byte_counter++] = byte_buffer;
				
				// Finish receiving when data buffer is full
				if (byte_counter >= TRF_DATA_SIZE) {
					_trf_current_state = TRF_STATE_RECEIVED;
				}
			}
		}
	}
	
	prev_timer_ovf = current_timer_ovf;
}

#endif /* TRF_RX_DISABLED */

/************************************************************************/
/* Transmitter                                                          */
/************************************************************************/

#ifndef TRF_TX_DISABLED

// Default TX pin is PB0
#ifndef TRF_TX_PIN
# define TRF_TX_PIN	PB0
#endif
#if !defined (TRF_RX_DISABLED) && TRF_TX_PIN == TRF_RX_PIN
# error "TRF_TX_PIN and TRF_RX_PIN must not be the same"
#endif

// How many times to send data
#ifndef TRF_TX_REPEAT_COUNT
# define TRF_TX_REPEAT_COUNT	10
#elif TRF_TX_REPEAT_COUNT <= 0
# error "TRF_TX_REPEAT_COUNT must be > 0"
#elif TRF_TX_REPEAT_COUNT < 5
# warning "TRF_TX_REPEAT_COUNT is too small. You may need to use higher values (>= 10) to improve reception stability"
#endif

// Set TRF_TX_PIN as output
#define _trf_set_tx_pin_out() DDRB |= (1 << TRF_TX_PIN)
// Change TRF_TX_PIN states
#define _trf_set_tx_pin_high() PORTB |= (1 << TRF_TX_PIN)
#define _trf_set_tx_pin_low() PORTB &= ~(1 << TRF_TX_PIN)

// Send single bit sequence
static void _trf_send_bit(uint8_t high_pulses, uint8_t all_pulses) {
	_trf_reset_timer();
	_trf_set_tx_pin_high();
	while (_trf_timer_ovf_count < high_pulses);
	_trf_set_tx_pin_low();
	while (_trf_timer_ovf_count < all_pulses);
}

// Send TRF_DATA_SIZE bytes from data array TRF_TX_REPEAT_COUNT times
static inline void trf_send(uint8_t *data) {
	// Disable all PCINTs during data transmission to prevent jitter
	uint8_t gimsk_state = GIMSK;
	GIMSK &= ~(1 << PCIE);
	
	// Send
	for (uint8_t i = 0; i < TRF_TX_REPEAT_COUNT; i++) {
		// Sync sequence
		_trf_send_bit(TRF_SYNC_HIGH_PULSES, TRF_SYNC_HIGH_PULSES + TRF_SYNC_LOW_PULSES);
		
		// Data bytes
		for (uint8_t j = 0; j < TRF_DATA_SIZE; j++) {
			uint8_t value = data[j];
			for (uint8_t l = 0; l < 8; l++) {
				if (value & 0x80) {
					// Send "1" bit
					_trf_send_bit(TRF_ONE_HIGH_PULSES, TRF_ONE_HIGH_PULSES + TRF_ONE_LOW_PULSES);
					} else {
					// Send "0" bit
					_trf_send_bit(TRF_ZERO_HIGH_PULSES, TRF_ZERO_HIGH_PULSES + TRF_ZERO_LOW_PULSES);
				}
				value <<= 1;
			}
		}
	}
	
	// Sync sequence at the end of transmission
	_trf_send_bit(TRF_SYNC_HIGH_PULSES, TRF_SYNC_HIGH_PULSES + TRF_SYNC_LOW_PULSES);
	
	// Restore PCINTs
	GIMSK = gimsk_state;
}

#endif /* TRF_TX_DISABLED */

// Common initialization
static inline void trf_init() {
	_trf_init_timer();
	
	#ifndef TRF_TX_DISABLED
	_trf_set_tx_pin_out();
	#endif
	
	#ifndef TRF_RX_DISABLED
	_trf_set_rx_pin_in();
	#endif
	
	// Enable interrupts
	sei();
}

#endif /* TINYRF_H_ */