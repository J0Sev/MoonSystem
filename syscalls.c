#include "syscalls.h"
#include <stdint.h>
#include <string.h>
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

void initGPIOCommunication(void) {
    uint32_t reg_val;
    uint32_t fsel_reg_addr;
    uint32_t fsel_mask;
    uint32_t fsel_value;
    
    // Configure GPIO_PIN_ZERO (17) as output
    // Pin 17 is controlled by GPFSEL1 register
    fsel_reg_addr = BCM2837_GPFSEL1;  // Physical address for pins 10-19
    reg_val = BCM2837_GET32(fsel_reg_addr);
    
    // Clear bits for pin 17 (bits 23-21 in GPFSEL1)
    fsel_mask = 0x7 << 21;  // 0b111 at bit position 21
    reg_val &= ~fsel_mask;
    
    // Set pin 17 to output (001)
    fsel_value = 0x1 << 21;
    reg_val |= fsel_value;
    
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Configure GPIO_PIN_ONE (27) as output
    // Pin 27 is controlled by GPFSEL2 register
    fsel_reg_addr = BCM2837_GPFSEL2;  // Physical address for pins 20-29
    reg_val = BCM2837_GET32(fsel_reg_addr);
    
    // Clear bits for pin 27 (bits 23-21 in GPFSEL2)
    fsel_mask = 0x7 << 21;  // 0b111 at bit position 21
    reg_val &= ~fsel_mask;
    
    // Set pin 27 to output (001)
    fsel_value = 0x1 << 21;
    reg_val |= fsel_value;
    
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Ensure both pins start low
    BCM2837_PUT32(BCM2837_GPCLR0, (1 << GPIO_PIN_ZERO) | (1 << GPIO_PIN_ONE));
}

void sendSignalToGM(char b) {
	uint8_t byte = (uint8_t)b;

	//Send each bit, Most Significant Bit first
	for (int i = 7; i >= 0; i--){
		if (byte & (1 << i)){
			//Bit is 1: pulse the "one" pin
			pulse_gpio(GPIO_PIN_ONE);
		} else {
			//Bit is 0: pulse the "zero" pin
			pulse_gpio(GPIO_PIN_ZERO);
		}
	}
}

void sendByteToGM(uint8_t b){
	//Send singular byte to GM
	sendSignalToGM((char)b);
}

void sendUint32ToGM(uint32_t value){
    //Send 32 bit value (Least significant byte first)
    sendByteToGM(value & 0xFF);           // Byte 0 (LSB)
    sendByteToGM((value >> 8) & 0xFF);    // Byte 1
    sendByteToGM((value >> 16) & 0xFF);   // Byte 2
    sendByteToGM((value >> 24) & 0xFF);   // Byte 3 (MSB)
}

void sendStringToGM(char* filename) {
    // Send each character until null terminator
    while (*filename != '\0') {
        sendSignalToGM(*filename);
        filename++;
    }
    // Send null terminator
    sendSignalToGM('\0');
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
    //Hang until GM sends the expected signal
    uint8_t received;
    do {
        received = getByteFromGM();
    } while (received != (uint8_t)b);
}

char* getStreamFromGM(uint8_t* numBytes) {
	//read numBytes bytes from GM into a buffer and return the pointer to the buffer
}

// Helper function to find free file slot
static FILE* allocate_file_slot(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_table[i].is_open) {
            return &file_table[i];
        }
    }
    return NULL;  // No free slots
}

// Helper function to parse mode string
static uint8_t parse_mode(const char* mode) {
    uint8_t flags = 0;
    
    while (*mode) {
        switch (*mode) {
            case 'r':
                flags |= MODE_READ;
                break;
            case 'w':
                flags |= MODE_WRITE;
                break;
            case 'a':
                flags |= MODE_APPEND;
                break;
            case '+':
                flags |= (MODE_READ | MODE_WRITE);
                break;
        }
        mode++;
    }
    
    return flags;
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