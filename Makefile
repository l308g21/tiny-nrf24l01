
# make sure it matches your attiny
MCU   ?= attiny85
F_CPU ?= 1000000UL
BAUD  ?= 38200UL


CC = avr-gcc
CFLAGS  = -Os -g -std=gnu99 -Wall
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections
CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD)

TARGET_ARCH = -mmcu=$(MCU)

all: src/libnrf24l01.a

src/nrf24l01.o:	src/nrf24l01.c src/nrf24l01.h src/nrf24l01-mnemonics.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

src/libnrf24l01.a: src/nrf24l01.o
	avr-ar rcs $@ $^

deploy: src/libnrf24l01.a
	cp $< /usr/avr/lib/
	cp src/*.h /usr/avr/include/

.PHONY: clear

clear:
	rm -f src/*.o src/*.a 