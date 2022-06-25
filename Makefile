
# make sure it matches your attiny
MCU   ?= attiny85
F_CPU ?= 1000000UL
BAUD  ?= 38200UL

TARGET_ARCH = -mmcu=$(MCU)

CC = avr-gcc
CFLAGS  = -Os -g -std=gnu99 -Wall
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections
CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD)



all: src/libtiny-nrf24l01.a

src/tiny-nrf24l01.o:	src/tiny-nrf24l01.c src/tiny-nrf24l01.h src/nrf24l01-mnemonics.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

src/libtiny-nrf24l01.a: src/tiny-nrf24l01.o
	avr-ar rcs $@ $^

deploy: src/libtiny-nrf24l01.a
	cp $< /usr/avr/lib/
	cp src/*.h /usr/avr/include/

.PHONY: clean

clean:
	rm -f src/*.o src/*.a 
