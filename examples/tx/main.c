#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include "nrf24l01.h"
#include "nrf24l01-mnemonics.h"

void setup_timer(void);


volatile bool rf_interrupt = false;
volatile bool send_message = false;
uint8_t counter;

int main(void) {
    uint8_t to_address[5] = { 0x62, 0x62, 0x62, 0x62, 0x62 };
    bool on = false;
    sei();
    setup_timer();
    nRF24L01_begin();

    while (true) {
        if (rf_interrupt) {
            rf_interrupt = false;
            DDRB |= (1 << PB3);
            PORTB |= (1 << PB3);
            int success = nRF24L01_transmit_success();
            if (success != 0)
                nRF24L01_flush_transmit_message();
            PORTB &= ~(1 << PB3);
            DDRB &= ~(1 << PB3);
        }

        if (send_message) {
            send_message = false;
            on = !on;
            nRF24L01Message msg;
            if (on) memcpy(msg.data, "ON", 3);
            else memcpy(msg.data, "OFF", 4);
            msg.length = strlen((char *)msg.data) + 1;
            nRF24L01_transmit(to_address, &msg);
        }
    }

    return 0;
}





// setup timer to trigger interrupt about every second when at 1MHz
void setup_timer(void) {

    TCCR0A &= ~( (1 << WGM00) | (1 << WGM01) );
    TCCR0B &= ~( (1 << WGM02) | (1 << CS01)  );
    // set prescaler 1:1024    (cs02 && cs00)
    TCCR0B |=  (1 << CS02) | (1<<CS00);
    // enable TCNT0 overflow interrupt
    TIMSK  |=  (1 << TOIE0);
}

// one second interrupt (1MHz * prescaler * 256 * counter)
ISR(TIMER0_OVF_vect) {
    if (counter == 4){
        send_message = true;
        counter = 0;
    }
    else counter++;
}

// nRF24L01_irq interrupt
ISR(INT0_vect) {
    rf_interrupt = true;
}