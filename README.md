AVR-nRF24L01+
=============

A library for using the nRF24L01+ with AVR chips like the ATtiny85, written in C.

You can find the equivalent ATmega library here: https://github.com/antoineleclair/avr-nrf24l01.

## Wiring
    ATtiny wiring:
                                                                       3.3V
                                                                        |
                     _______                                            R1
                   <| o     |>                                          |
                   <|       |> USCK/INT0---------------------o-----|<---o---o----------CSN
                   <|       |> D0---MOSI                     |     D1       |
                   <|       |> DI----o-------CE              |              --||----GND
                     _______         |                       |                C1
                                     R2                      |
                                     |                       o-----R3----IRQ
                                    MISO                     |
                                                             |
                                                            SCK
- **R1:** 51k
- **R2:** 20k
- **R3:** 5k
- **C1:** 680pF
- **D1:** 1N4148

## Changes to default register values

- **DYNPD:** 0x3f
- **FEATURE:** 0x06
- **RF_SETUP:** 0x07
- **RF_CH:** 0x4c

### Effects of default register value changes

- **1Mbps** data rate
- **dynamic payload length** enabled on all pipes
- active on **channel 0x4c**
