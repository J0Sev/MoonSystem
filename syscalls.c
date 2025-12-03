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
#define GPIO_PIN_ZERO 17 //Output: Send 0 bits
#define GPIO_PIN_ONE 27 //Output: Send 1 bits
#define GPIO_PIN_RX_ZERO 22 // Input: receive 0 bits
#define GPIO_PIN_RX_ONE 23 // Input: receive 1 bits
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
    fsel_reg_addr = BCM2837_GPFSEL1;
    reg_val = BCM2837_GET32(fsel_reg_addr);
    fsel_mask = 0x7 << 21;
    reg_val &= ~fsel_mask;
    fsel_value = 0x1 << 21;
    reg_val |= fsel_value;
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Configure GPIO_PIN_ONE (27) as output
    fsel_reg_addr = BCM2837_GPFSEL2;
    reg_val = BCM2837_GET32(fsel_reg_addr);
    fsel_mask = 0x7 << 21;
    reg_val &= ~fsel_mask;
    fsel_value = 0x1 << 21;
    reg_val |= fsel_value;
    BCM2837_PUT32(fsel_reg_addr, reg_val);
        
    // Configure GPIO_PIN_RX_ZERO (22) as input
    // Pin 22 is controlled by GPFSEL2 register
    fsel_reg_addr = BCM2837_GPFSEL2;
    reg_val = BCM2837_GET32(fsel_reg_addr);
    
    // Pin 22 position in GPFSEL2: (22 - 20) = 2, so bits 6-8
    fsel_mask = 0x7 << 6;  // 0b111 at bit position 6
    reg_val &= ~fsel_mask;  // Clear bits to 000 (INPUT mode)
    
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Configure GPIO_PIN_RX_ONE (23) as input
    // Pin 23 is also controlled by GPFSEL2 register
    reg_val = BCM2837_GET32(fsel_reg_addr);
    
    // Pin 23 position in GPFSEL2: (23 - 20) = 3, so bits 9-11
    fsel_mask = 0x7 << 9;  // 0b111 at bit position 9
    reg_val &= ~fsel_mask;  // Clear bits to 000 (INPUT mode)
    
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Set pull-down resistors for input pins this ensures the pins read LOW (0) when nothing is connected
    
    // Enable pull-down control
    BCM2837_PUT32(BCM2837_GPPUD, 0x1);  // 0x1 = pull-down, 0x2 = pull-up
    
    // Wait 150 cycles for the control signal to set up
    delay_us(1);
    
    // Clock the control signal into pins 22 and 23
    BCM2837_PUT32(BCM2837_GPPUDCLK0, (1 << GPIO_PIN_RX_ZERO) | (1 << GPIO_PIN_RX_ONE));
    
    // Wait 150 cycles
    delay_us(1);
    
    // Remove the control signal
    BCM2837_PUT32(BCM2837_GPPUD, 0);
    
    // Remove the clock
    BCM2837_PUT32(BCM2837_GPPUDCLK0, 0);
    
    //Initialize output pins to LOW
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

uint8_t getByteFromGM(void) {
    uint8_t result = 0;
    
    // Receive each bit, MSB first (same order as sending)
    for (int i = 7; i >= 0; i--) {
        uint8_t bit_received = 0;
        uint8_t pulse_detected = 0;
        
        // Wait for a pulse on either input pin
        while (!pulse_detected) {
            uint32_t pin_state = BCM2837_GET32(BCM2837_GPLEV0);
            
            // Check if pin RX_ONE is HIGH (bit is 1)
            if (pin_state & (1 << GPIO_PIN_RX_ONE)) {
                bit_received = 1;
                pulse_detected = 1;
                
                // Wait for the pin to go back LOW (end of pulse)
                while (BCM2837_GET32(BCM2837_GPLEV0) & (1 << GPIO_PIN_RX_ONE)) {
                    // Busy wait
                }
            }
            // Check if pin RX_ZERO is HIGH (bit is 0)
            else if (pin_state & (1 << GPIO_PIN_RX_ZERO)) {
                bit_received = 0;
                pulse_detected = 1;
                
                // Wait for the pin to go back LOW (end of pulse)
                while (BCM2837_GET32(BCM2837_GPLEV0) & (1 << GPIO_PIN_RX_ZERO)) {
                    // Busy wait
                }
            }
        }
        
        // Set the bit in the result if it was 1
        if (bit_received) {
            result |= (1 << i);
        }
        
        // Small delay before reading next bit
        delay_us(SIGNAL_DELAY / 2);
    }
    
    return result;
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
    //Receive the number of bytes first
    uint8_t count = getByteFromGM();
    *numBytes = count;
    
    //Allocate buffer
    static char buffer[256];  // Static buffer (limit: 256 bytes)
    
    //Read bytes into buffer
    for (uint8_t i = 0; i < count; i++) {
        buffer[i] = (char)getByteFromGM();
    }
    
    return buffer;
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
    // Find a free file slot
    FILE* file = allocate_file_slot();
    if (!file) {
        return NULL;  // No available slots
    }
    
    // Send the fopen signal to GM
    sendSignalToGM(FOPEN_SIGNAL);

    // Wait for GM to request the next argument
    awaitSignalFromGM(NEXT_ARG);

    // Send filename to GM
    sendStringToGM(filename);

    // Wait for GM to request mode
    awaitSignalFromGM(NEXT_ARG);

    // Send mode to GM
    sendStringToGM(mode);

    // Wait for ACK from GM
    awaitSignalFromGM(ACK);

    // Receive file handle from GM
    uint32_t file_handle = getUint32FromGM();
    
    // Receive file size from GM
    uint32_t file_size = getUint32FromGM();

    // Initialize the FILE structure
    file->is_open = 1;
    file->mode = parse_mode(mode);
    file->current_cluster = file_handle;
    file->file_size = file_size;
    file->position = 0;
    file->buffer_dirty = 0;
    file->buffer_cluster = 0;
    
    // Copy filename
    strncpy(file->filename, filename, FILENAME_MAX_LEN - 1);
    file->filename[FILENAME_MAX_LEN - 1] = '\0';

    return file;    
}

int fclose(FILE *stream) {
    if (!stream || !stream->is_open) {
        return -1;  // Invalid stream
    }
    
    // Send the fclose signal to GM
    sendSignalToGM(FCLOSE_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Mark file as closed
    stream->is_open = 0;
    
    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
	//send the fwrite signal to GM, wait for the confirmation signal from GM, then send the data to GM
        if (!stream || !stream->is_open || !(stream->mode & MODE_WRITE)) {
        return 0;  // Invalid stream or not writable
    }
    
    size_t total_bytes = size * count;
    const uint8_t* data = (const uint8_t*)ptr;
    
    // Send the fwrite signal to GM
    sendSignalToGM(FWRITE_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for GM to request byte count
    awaitSignalFromGM(NEXT_ARG);
    
    // Send number of bytes to write
    sendUint32ToGM(total_bytes);
    
    // Wait for GM to request data
    awaitSignalFromGM(NEXT_ARG);
    
    // Send the actual data
    sendBytesToGM(data, total_bytes);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Receive number of bytes actually written
    uint32_t bytes_written = getUint32FromGM();
    
    // Update position
    stream->position += bytes_written;
    if (stream->position > stream->file_size) {
        stream->file_size = stream->position;
    }
    
    return bytes_written / size;  // Return number of items written
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
	//send the fread signal to GM, read the data incoming from GM
        if (!stream || !stream->is_open || !(stream->mode & MODE_READ)) {
        return 0;  // Invalid stream or not readable
    }
    
    size_t total_bytes = size * count;
    uint8_t* data = (uint8_t*)ptr;
    
    // Send the fread signal to GM
    sendSignalToGM(FREAD_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for GM to request byte count
    awaitSignalFromGM(NEXT_ARG);
    
    // Send number of bytes to read
    sendUint32ToGM(total_bytes);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Receive number of bytes that will be sent
    uint32_t bytes_to_receive = getUint32FromGM();
    
    // Receive the actual data
    getBytesFromGM(data, bytes_to_receive);
    
    // Update position
    stream->position += bytes_to_receive;
    
    return bytes_to_receive / size;  // Return number of items read

}

int fseek(FILE *stream, long offset, int whence) {
    if (!stream || !stream->is_open) {
        return -1;  // Invalid stream
    }
    
    // Send the fseek signal to GM
    sendSignalToGM(FSEEK_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for GM to request offset
    awaitSignalFromGM(NEXT_ARG);
    
    // Send offset (as 32-bit value)
    sendUint32ToGM((uint32_t)offset);
    
    // Wait for GM to request whence
    awaitSignalFromGM(NEXT_ARG);
    
    // Send whence
    sendByteToGM((uint8_t)whence);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Receive new position
    stream->position = getUint32FromGM();
    
    return 0;  // Success
}

long ftell(FILE *stream) {
    if (!stream || !stream->is_open) {
        return -1L;  // Invalid stream
    }
    
    // Send the ftell signal to GM
    sendSignalToGM(FTELL_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Receive current position
    uint32_t position = getUint32FromGM();
    stream->position = position;
    
    return (long)position;
}

int feof(FILE *stream) {
    if (!stream || !stream->is_open) {
        return 1;  // Invalid stream = EOF
    }
    
    // Send the feof signal to GM
    sendSignalToGM(FEOF_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Receive EOF status (0 = not EOF, 1 = EOF)
    uint8_t eof_status = getByteFromGM();
    
    return eof_status;
}

void rewind(FILE *stream) {
    if (!stream || !stream->is_open) {
        return;  // Invalid stream
    }
    
    // Send the rewind signal to GM
    sendSignalToGM(REWIND_SIGNAL);
    
    // Wait for GM to request file handle
    awaitSignalFromGM(NEXT_ARG);
    
    // Send file handle
    sendUint32ToGM(stream->current_cluster);
    
    // Wait for ACK
    awaitSignalFromGM(ACK);
    
    // Reset position
    stream->position = 0;
}

// File system initialization
int fs_init(void) {
    // Initialize GPIO communication
    initGPIOCommunication();
    
    // Clear file table
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        file_table[i].is_open = 0;
    }
    
    return 0;  // Success
}

int main() {

}