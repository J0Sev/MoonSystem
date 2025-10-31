#include "syscalls.h"
#include <stdint.h>
#include "BCM2837.h"

//Signal Codes
#define FOPEN_SIGNAL    0
#define FCLOSE_SIGNAL   1
#define FREAD_SIGNAL    2
#define FWRITE_SIGNAL   3
#define FSEEK_SIGNAL    4
#define FTELL_SIGNAL    5
#define FEOF_SIGNAL     6
#define REWIND_SIGNAL   7

//Protocol Control Codes
#define NEXT_ARG        0x10
#define ACK             0x11

//GPIO Pin signal definitions
#define GPIO_PIN_ZERO 17
#define GPIO_PIN_ONE 27
#define SIGNAL_DELAY 100

//Global file table to store data
static FILE file_table[MAX_OPEN_FILES];

//Timed delay function
static void delay_us(uint32_t microsecs){
	volatile uint32_t count = microsecs * 50;
	while (count--) {
	}
}

//Helper function to pulse gpio pin
static void pulse_gpio(uint32_t pin){
	//Set pin high
	BCM2837_PUT32(BCM2837_GPSET0, (1 << pin));
	delay_us(SIGNAL_DELAY);

	//Set pin low
	BCM2837_PUT32(BCM2837_GPCLR0, (1 << pin));
	delay_us(SIGNAL_DELAY);
}

void sendSignalToGM(char b) {

}

void sendByteToGM(uint8_t b){
	//Send singular byte to GM
}

void sendUint32ToGM(uint32_t value){
	//Send 32 bit value
}

void sendStringToGM(char* filename) {
	int x;
	for (int i=0; filename[i] != '\0'; ++i) {
		sendSignalToGM(filename[i]);

		//hang for a milisecond or so
		for (int j=0; j<1000000; ++j) 
			x = 0;
		
	}
}

void sendBytesToGM(const uint8_t* data, size_t len){
	for (size_t i = 0; i < len; i++){
		sendByteToGM(data[i]);
	}
}

uint32_t getUint32FromGM(void) {
    uint32_t value = 0;
    value |= ((uint32_t)getByteFromGM()) << 0;
    value |= ((uint32_t)getByteFromGM()) << 8;
    value |= ((uint32_t)getByteFromGM()) << 16;
    value |= ((uint32_t)getByteFromGM()) << 24;
    return value;
}

void getBytesFromGM(uint8_t* buffer, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer[i] = getByteFromGM();
    }
} 

void awaitSignalFromGM(char b) {
	//hang until GM sends b signal
}

char* getStreamFromGM(uint8_t* numBytes) {
	//read numBytes bytes from GM into a buffer and return the pointer to the buffer
}


FILE* fopen(const char *filename, const char *mode) {
	//send the fopen signal to GM
	sendSignalToGM(FOPEN_SIGNAL);

	//wait for GM to request the next argument to fopen
	awaitSignalFromGM(NEXT_ARG);

	//send filename to GM
	sendStringToGM(filename);

	char* buffer = char[8];
	buffer = sendSignalToGM(mode);

	return getStreamFromGM(8);	
}

int fclose(FILE *stream) {
	//send the fclose signal to GM

}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
	//send the fwrite signal to GM, wait for the confirmation signal from GM, then send the data to GM

}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
	//send the fread signal to GM, read the data incoming from GM

}

int fseek(FILE *stream, long offset, int whence) {

}

long ftell(FILE *stream) {

}




int main() {

}