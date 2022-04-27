#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef _NRF24L01_H
#define _NRF24L01_H

typedef struct {
    int pipe_number;
    uint8_t data[32];
    uint8_t length;
} nRF24L01Message;

extern uint8_t nRF24L01_status;


void nRF24L01_begin();
uint8_t nRF24L01_send_command(uint8_t command, void *data, size_t length);
uint8_t nRF24L01_write_register(uint8_t reg_address, void *data, size_t length);
uint8_t nRF24L01_read_register(uint8_t regAddress, void *data, size_t length);
uint8_t nRF24L01_no_op();
uint8_t nRF24L01_update_status();
uint8_t nRF24L01_get_status();
void nRF24L01_listen(int pipe, uint8_t *address);
bool nRF24L01_data_received();
bool nRF24L01_read_received_data(nRF24L01Message *message);
int nRF24L01_pipe_number_received();
void nRF24L01_transmit(void *address, nRF24L01Message *msg);
int nRF24L01_transmit_success();
void nRF24L01_flush_transmit_message();
void nRF24L01_retry_transmit();
void nRF24L01_clear_interrupts();
void nRF24L01_clear_transmit_interrupts();
void nRF24L01_clear_receive_interrupt();

#endif
