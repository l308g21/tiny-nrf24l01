#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "tiny-nrf24l01.h"
#include "nrf24l01-mnemonics.h"


static          void    copy_address    (uint8_t *source, uint8_t *destination);
static inline   void    set_csn_high    ();
static inline   void    set_csn_low     ();
static          void    spi_init        ();
static          uint8_t spi_transfer    (uint8_t data);
static inline   void    int0_init       ();
static inline   void    set_int0_input  ();
static inline   void    set_sck_output  ();
static inline   void    pulse_ce        ();
static inline   void    set_ce_high     ();
static inline   void    set_di_input    ();


uint8_t nRF24L01_status;
bool    nRF24L01_rx_mode = false;

void nRF24L01_begin() {
    spi_init();
    set_sck_output();
    nRF24L01_send_command(FLUSH_RX, NULL, 0);
    nRF24L01_send_command(FLUSH_TX, NULL, 0);
    nRF24L01_clear_interrupts();

    uint8_t data;
    data = _BV(EN_CRC) | _BV(CRCO) | _BV(PWR_UP) | _BV(PRIM_RX);
    nRF24L01_write_register(CONFIG, &data, 1);

    // enable Auto Acknowlegde on all pipes
    data = _BV(ENAA_P0) | _BV(ENAA_P1) | _BV(ENAA_P2)
         | _BV(ENAA_P3) | _BV(ENAA_P4) | _BV(ENAA_P5);
    nRF24L01_write_register(EN_AA, &data, 1);

    // enable Dynamic Payload on all pipes
    data = _BV(DPL_P0) | _BV(DPL_P1) | _BV(DPL_P2)
         | _BV(DPL_P3) | _BV(DPL_P4) | _BV(DPL_P5);
    nRF24L01_write_register(DYNPD, &data, 1);

    // set channel
    data = 0x4c;
    nRF24L01_write_register(RF_CH, &data, 1);
    
    // enable Dynamic Payload (global)
    data = _BV(EN_DPL) | (1 << EN_ACK_PAY);
    nRF24L01_write_register(FEATURE, &data, 1);

    // disable all rx addresses
    data = 0;
    nRF24L01_write_register(EN_RXADDR, &data, 1);
    // set data rate to 1Mbps
    data = (1 << RF_PWR1) | (1 << RF_PWR2) | (1 << LNA_HCURR);
    nRF24L01_write_register(RF_SETUP, &data, 1);
    
    int0_init();
}



uint8_t nRF24L01_send_command(uint8_t command, void *data,
    size_t length) {
    set_di_input();
    set_sck_output();
    set_csn_high();
    set_csn_low ();

    nRF24L01_status = spi_transfer(command);
    for (unsigned int i = 0; i < length; i++)
        ((uint8_t*)data)[i] = spi_transfer(((uint8_t*)data)[i]);

    PORTB |= (1 << PB2);
    set_int0_input();
    if (nRF24L01_rx_mode)
        set_ce_high();
    return nRF24L01_status;
}


uint8_t nRF24L01_write_register(uint8_t reg_address, void *data, size_t length) {
    return nRF24L01_send_command(W_REGISTER | reg_address, data, length);
}

uint8_t nRF24L01_read_register(uint8_t reg_address, void *data, size_t length) {
    return nRF24L01_send_command(R_REGISTER | reg_address, data, length);
}

uint8_t nRF24L01_no_op() {
    return nRF24L01_send_command(NOP, NULL, 0);
}

uint8_t nRF24L01_update_status() {
    return nRF24L01_no_op();
}

uint8_t nRF24L01_get_status() {
    return nRF24L01_status;
}

bool nRF24L01_data_received() {
    nRF24L01_update_status();
    return nRF24L01_pipe_number_received() >= 0;
}




void nRF24L01_listen(int pipe, uint8_t *address) {
    uint8_t addr[5];
    copy_address(address, addr);

    nRF24L01_write_register(RX_ADDR_P0 + pipe, addr, 5);

    uint8_t current_pipes;
    nRF24L01_read_register(EN_RXADDR, &current_pipes, 1);
    current_pipes |= _BV(pipe);
    nRF24L01_write_register(EN_RXADDR, &current_pipes, 1);

    nRF24L01_rx_mode = true;
}


bool nRF24L01_read_received_data(nRF24L01Message *message) {
    message->pipe_number = nRF24L01_pipe_number_received();
    nRF24L01_clear_receive_interrupt();
    if (message->pipe_number < 0) {
        message->length = 0;
        return false;
    }

    nRF24L01_read_register(R_RX_PL_WID, &message->length, 1);

    if (message->length > 0) {
        nRF24L01_send_command(R_RX_PAYLOAD, &message->data,
            message->length);
    }

    return true;
}


int nRF24L01_pipe_number_received() {
    int pipe_number = (nRF24L01_status & RX_P_NO_MASK) >> 1;
    return pipe_number <= 5 ? pipe_number : -1;
}




void nRF24L01_transmit(void *address, nRF24L01Message *msg) {
    // keep ce low
    nRF24L01_rx_mode = false;
    nRF24L01_clear_transmit_interrupts();
    nRF24L01_flush_transmit_message();
    uint8_t addr[5];
    copy_address((uint8_t *)address, addr);
    nRF24L01_write_register(TX_ADDR, addr, 5);
    nRF24L01_write_register(RX_ADDR_P0, addr, 5);
    nRF24L01_send_command(W_TX_PAYLOAD, &msg->data, msg->length);
    uint8_t config;
    nRF24L01_read_register(CONFIG, &config, 1);
    config &= ~_BV(PRIM_RX);
    nRF24L01_write_register(CONFIG, &config, 1);
    
    pulse_ce();
}


int nRF24L01_transmit_success() {
    nRF24L01_update_status();
    int success;
    if (nRF24L01_status & _BV(TX_DS)) success = 0;
    else if (nRF24L01_status & _BV(MAX_RT)) success = -1;
    else success = -2;
    nRF24L01_clear_transmit_interrupts();
    uint8_t config;
    nRF24L01_read_register(CONFIG, &config, 1);
    config |= _BV(PRIM_RX);
    nRF24L01_write_register(CONFIG, &config, 1);
    return success;
}


void nRF24L01_flush_transmit_message() {
    nRF24L01_send_command(FLUSH_TX, NULL, 0);
}


void nRF24L01_retry_transmit() {
    // XXX not sure it works this way, never tested
    uint8_t config;
    nRF24L01_read_register(CONFIG, &config, 1);
    config &= ~_BV(PRIM_RX);
    nRF24L01_write_register(CONFIG, &config, 1);
}


void nRF24L01_clear_interrupts() {
    uint8_t data = _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT);
    nRF24L01_write_register(STATUS, &data, 1);
}


void nRF24L01_clear_transmit_interrupts() {
    uint8_t data = _BV(TX_DS) | _BV(MAX_RT);
    nRF24L01_write_register(STATUS, &data, 1);
}


void nRF24L01_clear_receive_interrupt() {
    uint8_t data = _BV(RX_DR) | nRF24L01_status;
    nRF24L01_write_register(STATUS, &data, 1);
}




static void copy_address(uint8_t *source, uint8_t *destination) {
    for (int i = 0; i < 5; i++)
        destination[i] = source[i];
}




static inline void int0_init(){
    // enable interrupt if INT0 = 0; default pin-state: 1 via nRF24L01_IRQ pin's internal pullup resistor
    MCUCR &= ~( (1 << ISC01) | (1 << ISC00) );
    GIMSK |=  (1 << INT0);
    return;
}

static inline void set_int0_input(){
    // multiplexed PB2 - is now input
    GIMSK |=  (1 << INT0);
    DDRB  &= ~(1 << PB2);
    return;
}

static inline void set_sck_output(){
    // disable INTO interrupt to avoid software generated interrupt while operating as sck
    GIMSK &= ~(1 << INT0);
    // sck high by default - set PB2 high to avoid unintentional toggle
    PORTB |=  (1 << PB2);
    DDRB  |=  (1 << PB2);
    return;
}

static inline void set_csn_high(){
    PORTB |= (1 << PB2);
    _delay_us(32);
    return;
}

static inline void set_csn_low(){
    PORTB &= ~(1 << PB2);
    _delay_us(4);
    return;
}




static inline void pulse_ce(){
    // low high low pulse > 10us
    set_ce_high();
    _delay_us(11);
    set_di_input();
    return;
}

static inline void set_ce_high(){
    DDRB  |= (1 << PB0);
    PORTB |= (1 << PB0);
    return;
}

static inline void set_di_input(){
    PORTB &= ~(1 << PB0);
    DDRB  &= ~(1 << PB0);
    return;
}




static void spi_init() {
    // enable three wire mode (spi)
    USICR |=  (1 << USIWM0);
    USICR &= ~(1 << USIWM1);
    // set DO (PB1) && SCK (PB2) as output and DI (PB0) as input
    DDRB  |=  (1 << PB1) | (1 << PB2);
    DDRB  &= ~(1 << PB0);
    // set sck source to software strobe
    USICR |=  (1 << USICS1) | (1 << USICLK);
    USICR &= ~(1 << USICS0);
    return;
}

static uint8_t spi_transfer(uint8_t data) {
    // load data
    USIDR = data;
    // clear usi counter overflow flag
    USISR = (1<<USIOIF);
    while   ( !(USISR & (1 << USIOIF)) )    USICR |= (1 << USITC);
    return  USIDR;
}
