#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"


void process_message(char *message);
static inline void prepare_led_pin(void);
static inline void  set_led_high(void);
static inline void  set_led_low(void);

volatile bool rf_interrupt = false;


int main(void) {
    uint8_t address[5] = { 0x62, 0x62, 0x62, 0x62, 0x62 };
    prepare_led_pin();
    sei();
    nRF24L01_begin();
    nRF24L01_listen(0, address);
    uint8_t addr[5];
    nRF24L01_read_register(CONFIG, addr, 1);

    while (true) {
        if (rf_interrupt) {
            rf_interrupt = false;
            while (nRF24L01_data_received()) {
                nRF24L01Message msg;
                nRF24L01_read_received_data(&msg);
                process_message((char *)msg.data);
            }

            nRF24L01_listen(0, address);
        }
    }

    return 0;
}


void process_message(char *message) {
    if (strcmp(message, "ON") == 0)
        set_led_high();
    else if (strcmp(message, "OFF") == 0)
        set_led_low();
}

static inline void prepare_led_pin(void) {
    DDRB |= _BV(PB3);
    PORTB &= ~_BV(PB3);
}

static inline void set_led_high(void) {
    PORTB |= _BV(PB3);
}

static inline void set_led_low(void) {
    PORTB &= ~_BV(PB3);
}

// nRF24L01 interrupt
ISR(INT0_vect) {
    rf_interrupt = true;
}
