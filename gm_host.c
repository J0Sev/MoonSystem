#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#ifdef __linux__
    #include <unistd.h>
    #include "BCM2837.h"
    #define USING_GPIO 1
#else
    // Windows - simulate GPIO for testing
    #define USING_GPIO 0
    #warning "Building for Windows - GPIO functions will be simulated"
    
    // Dummy BCM2837 definitions for Windows compilation
    #define BCM2837_GPSET0 0
    #define BCM2837_GPCLR0 0
    #define BCM2837_GPLEV0 0
    #define BCM2837_GPFSEL1 0
    #define BCM2837_GPFSEL2 0
    #define BCM2837_GPPUD 0
    #define BCM2837_GPPUDCLK0 0
    
    #define BCM2837_PUT32(addr, val) do {} while(0)
    #define BCM2837_GET32(addr) (0)
#endif

//Signal Codes (must match Pi side)
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

//GPIO Pin Configuration (from GM's perspective)
#define GPIO_PIN_RX_ZERO 17  // Input: receive 0 bits FROM Pi
#define GPIO_PIN_RX_ONE 27   // Input: receive 1 bits FROM Pi
#define GPIO_PIN_ZERO 22     // Output: send 0 bits TO Pi
#define GPIO_PIN_ONE 23      // Output: send 1 bits TO Pi

#define SIGNAL_DELAY 100
#define MAX_OPEN_FILES 16
#define FILENAME_MAX_LEN 256
#define BCM2837_GPLEV0  0x3F200034

// File handle table
typedef struct {
    FILE* file;
    uint32_t handle;
    uint8_t is_open;
} GMFileEntry;

static GMFileEntry file_table[MAX_OPEN_FILES];
static uint32_t next_handle = 1;

//GPIO Communication Functions
static void delay_us(uint32_t microsecs) {
    volatile uint32_t count = microsecs * 50;
    while (count--) {}
}

static void pulse_gpio(uint32_t pin) {
    BCM2837_PUT32(BCM2837_GPSET0, (1 << pin));
    delay_us(SIGNAL_DELAY);
    BCM2837_PUT32(BCM2837_GPCLR0, (1 << pin));
    delay_us(SIGNAL_DELAY);
}

void initGPIOCommunication(void) {
    uint32_t reg_val;
    uint32_t fsel_reg_addr;
    uint32_t fsel_mask;
    uint32_t fsel_value;
    
    printf("[GM] Initializing GPIO communication...\n");
    
    // Configure GPIO_PIN_ZERO (22) as output
    fsel_reg_addr = BCM2837_GPFSEL2;
    reg_val = BCM2837_GET32(fsel_reg_addr);
    fsel_mask = 0x7 << 6;  // Pin 22: bits 6-8
    reg_val &= ~fsel_mask;
    fsel_value = 0x1 << 6;
    reg_val |= fsel_value;
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Configure GPIO_PIN_ONE (23) as output
    reg_val = BCM2837_GET32(fsel_reg_addr);
    fsel_mask = 0x7 << 9;  // Pin 23: bits 9-11
    reg_val &= ~fsel_mask;
    fsel_value = 0x1 << 9;
    reg_val |= fsel_value;
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Configure GPIO_PIN_RX_ZERO (17) as input
    fsel_reg_addr = BCM2837_GPFSEL1;
    reg_val = BCM2837_GET32(fsel_reg_addr);
    fsel_mask = 0x7 << 21;  // Pin 17: bits 21-23
    reg_val &= ~fsel_mask;  // Clear to 000 (input)
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Configure GPIO_PIN_RX_ONE (27) as input
    fsel_reg_addr = BCM2837_GPFSEL2;
    reg_val = BCM2837_GET32(fsel_reg_addr);
    fsel_mask = 0x7 << 21;  // Pin 27: bits 21-23
    reg_val &= ~fsel_mask;  // Clear to 000 (input)
    BCM2837_PUT32(fsel_reg_addr, reg_val);
    
    // Set pull-down resistors for input pins
    BCM2837_PUT32(BCM2837_GPPUD, 0x1);
    delay_us(1);
    BCM2837_PUT32(BCM2837_GPPUDCLK0, (1 << GPIO_PIN_RX_ZERO) | (1 << GPIO_PIN_RX_ONE));
    delay_us(1);
    BCM2837_PUT32(BCM2837_GPPUD, 0);
    BCM2837_PUT32(BCM2837_GPPUDCLK0, 0);
    
    // Initialize output pins to LOW
    BCM2837_PUT32(BCM2837_GPCLR0, (1 << GPIO_PIN_ZERO) | (1 << GPIO_PIN_ONE));
    
    printf("[GM] GPIO initialized\n");
}

void sendByteToPI(uint8_t b) {
    uint8_t byte = b;
    for (int i = 7; i >= 0; i--) {
        if (byte & (1 << i)) {
            pulse_gpio(GPIO_PIN_ONE);
        } else {
            pulse_gpio(GPIO_PIN_ZERO);
        }
    }
}

void sendUint32ToPi(uint32_t value) {
    sendByteToPI(value & 0xFF);
    sendByteToPI((value >> 8) & 0xFF);
    sendByteToPI((value >> 16) & 0xFF);
    sendByteToPI((value >> 24) & 0xFF);
}

void sendBytesToPi(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sendByteToPI(data[i]);
    }
}

uint8_t getByteFromPi(void) {
    uint8_t result = 0;
    
    for (int i = 7; i >= 0; i--) {
        uint8_t bit_received = 0;
        uint8_t pulse_detected = 0;
        
        while (!pulse_detected) {
            uint32_t pin_state = BCM2837_GET32(BCM2837_GPLEV0);
            
            if (pin_state & (1 << GPIO_PIN_RX_ONE)) {
                bit_received = 1;
                pulse_detected = 1;
                while (BCM2837_GET32(BCM2837_GPLEV0) & (1 << GPIO_PIN_RX_ONE)) {}
            }
            else if (pin_state & (1 << GPIO_PIN_RX_ZERO)) {
                bit_received = 0;
                pulse_detected = 1;
                while (BCM2837_GET32(BCM2837_GPLEV0) & (1 << GPIO_PIN_RX_ZERO)) {}
            }
        }
        
        if (bit_received) {
            result |= (1 << i);
        }
        
        delay_us(SIGNAL_DELAY / 2);
    }
    
    return result;
}

uint32_t getUint32FromPi(void) {
    uint32_t value = 0;
    value |= ((uint32_t)getByteFromPi()) << 0;
    value |= ((uint32_t)getByteFromPi()) << 8;
    value |= ((uint32_t)getByteFromPi()) << 16;
    value |= ((uint32_t)getByteFromPi()) << 24;
    return value;
}

void getStringFromPi(char* buffer, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1) {
        buffer[i] = (char)getByteFromPi();
        if (buffer[i] == '\0') {
            break;
        }
        i++;
    }
    buffer[i] = '\0';
}

void getBytesFromPi(uint8_t* buffer, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer[i] = getByteFromPi();
    }
}

// File Table Management

GMFileEntry* findFileByHandle(uint32_t handle) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_table[i].is_open && file_table[i].handle == handle) {
            return &file_table[i];
        }
    }
    return NULL;
}

GMFileEntry* allocateFileEntry(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_table[i].is_open) {
            return &file_table[i];
        }
    }
    return NULL;
}

// Protocol Handlers

void handle_fopen(void) {
    char filename[FILENAME_MAX_LEN];
    char mode[8];
    
    printf("[GM] FOPEN request\n");
    
    // Request filename
    sendByteToPI(NEXT_ARG);
    getStringFromPi(filename, FILENAME_MAX_LEN);
    printf("[GM]   Filename: %s\n", filename);
    
    // Request mode
    sendByteToPI(NEXT_ARG);
    getStringFromPi(mode, sizeof(mode));
    printf("[GM]   Mode: %s\n", mode);
    
    // Open the file
    FILE* f = fopen(filename, mode);
    
    if (f) {
        // Allocate file entry
        GMFileEntry* entry = allocateFileEntry();
        if (entry) {
            entry->file = f;
            entry->handle = next_handle++;
            entry->is_open = 1;
            
            // Get file size
            fseek(f, 0, SEEK_END);
            uint32_t file_size = (uint32_t)ftell(f);
            fseek(f, 0, SEEK_SET);
            
            // Send ACK
            sendByteToPI(ACK);
            
            // Send file handle
            sendUint32ToPi(entry->handle);
            
            // Send file size
            sendUint32ToPi(file_size);
            
            printf("[GM]   Success! Handle: %u, Size: %u\n", entry->handle, file_size);
        } else {
            printf("[GM]   Error: No free file slots\n");
            // Send error (handle 0 means failure)
            sendByteToPI(ACK);
            sendUint32ToPi(0);
            sendUint32ToPi(0);
            fclose(f);
        }
    } else {
        printf("[GM]   Error: Failed to open file\n");
        // Send error
        sendByteToPI(ACK);
        sendUint32ToPi(0);
        sendUint32ToPi(0);
    }
}

void handle_fclose(void) {
    printf("[GM] FCLOSE request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    printf("[GM]   Handle: %u\n", handle);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        fclose(entry->file);
        entry->is_open = 0;
        sendByteToPI(ACK);
        printf("[GM]   Success!\n");
    } else {
        sendByteToPI(ACK);
        printf("[GM]   Error: Invalid handle\n");
    }
}

void handle_fwrite(void) {
    printf("[GM] FWRITE request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    // Request byte count
    sendByteToPI(NEXT_ARG);
    uint32_t byte_count = getUint32FromPi();
    
    printf("[GM]   Handle: %u, Bytes: %u\n", handle, byte_count);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        // Request data
        sendByteToPI(NEXT_ARG);
        
        // Allocate temporary buffer
        uint8_t* buffer = (uint8_t*)malloc(byte_count);
        if (buffer) {
            getBytesFromPi(buffer, byte_count);
            
            // Write to file
            size_t written = fwrite(buffer, 1, byte_count, entry->file);
            fflush(entry->file);
            
            free(buffer);
            
            // Send ACK
            sendByteToPI(ACK);
            
            // Send bytes written
            sendUint32ToPi((uint32_t)written);
            
            printf("[GM]   Wrote %zu bytes\n", written);
        } else {
            printf("[GM]   Error: Memory allocation failed\n");
            sendByteToPI(ACK);
            sendUint32ToPi(0);
        }
    } else {
        printf("[GM]   Error: Invalid handle\n");
        sendByteToPI(ACK);
        sendUint32ToPi(0);
    }
}

void handle_fread(void) {
    printf("[GM] FREAD request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    // Request byte count
    sendByteToPI(NEXT_ARG);
    uint32_t byte_count = getUint32FromPi();
    
    printf("[GM]   Handle: %u, Bytes: %u\n", handle, byte_count);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        // Allocate buffer
        uint8_t* buffer = (uint8_t*)malloc(byte_count);
        if (buffer) {
            // Read from file
            size_t bytes_read = fread(buffer, 1, byte_count, entry->file);
            
            // Send ACK
            sendByteToPI(ACK);
            
            // Send byte count
            sendUint32ToPi((uint32_t)bytes_read);
            
            // Send data
            sendBytesToPi(buffer, bytes_read);
            
            free(buffer);
            
            printf("[GM]   Read %zu bytes\n", bytes_read);
        } else {
            printf("[GM]   Error: Memory allocation failed\n");
            sendByteToPI(ACK);
            sendUint32ToPi(0);
        }
    } else {
        printf("[GM]   Error: Invalid handle\n");
        sendByteToPI(ACK);
        sendUint32ToPi(0);
    }
}

void handle_fseek(void) {
    printf("[GM] FSEEK request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    // Request offset
    sendByteToPI(NEXT_ARG);
    uint32_t offset = getUint32FromPi();
    
    // Request whence
    sendByteToPI(NEXT_ARG);
    uint8_t whence = getByteFromPi();
    
    printf("[GM]   Handle: %u, Offset: %d, Whence: %u\n", handle, (int32_t)offset, whence);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        fseek(entry->file, (long)offset, whence);
        long new_pos = ftell(entry->file);
        
        // Send ACK
        sendByteToPI(ACK);
        
        // Send new position
        sendUint32ToPi((uint32_t)new_pos);
        
        printf("[GM]   New position: %ld\n", new_pos);
    } else {
        printf("[GM]   Error: Invalid handle\n");
        sendByteToPI(ACK);
        sendUint32ToPi(0);
    }
}

void handle_ftell(void) {
    printf("[GM] FTELL request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    printf("[GM]   Handle: %u\n", handle);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        long pos = ftell(entry->file);
        
        // Send ACK
        sendByteToPI(ACK);
        
        // Send position
        sendUint32ToPi((uint32_t)pos);
        
        printf("[GM]   Position: %ld\n", pos);
    } else {
        printf("[GM]   Error: Invalid handle\n");
        sendByteToPI(ACK);
        sendUint32ToPi(0);
    }
}

void handle_feof(void) {
    printf("[GM] FEOF request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    printf("[GM]   Handle: %u\n", handle);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        int eof_status = feof(entry->file);
        
        // Send ACK
        sendByteToPI(ACK);
        
        // Send EOF status
        sendByteToPI((uint8_t)eof_status);
        
        printf("[GM]   EOF: %d\n", eof_status);
    } else {
        printf("[GM]   Error: Invalid handle\n");
        sendByteToPI(ACK);
        sendByteToPI(1);  // Return EOF for invalid handle
    }
}

void handle_rewind(void) {
    printf("[GM] REWIND request\n");
    
    // Request file handle
    sendByteToPI(NEXT_ARG);
    uint32_t handle = getUint32FromPi();
    
    printf("[GM]   Handle: %u\n", handle);
    
    GMFileEntry* entry = findFileByHandle(handle);
    if (entry) {
        rewind(entry->file);
        sendByteToPI(ACK);
        printf("[GM]   Success!\n");
    } else {
        printf("[GM]   Error: Invalid handle\n");
        sendByteToPI(ACK);
    }
}

int main(void) {
    printf("GM File System Handler\n");
    
    #if USING_GPIO
        printf("Mode: Real GPIO (Raspberry Pi)\n");
    #else
        printf("Mode: Simulation (Windows)\n");
        printf("WARNING: GPIO functions are disabled!\n");
    #endif
        
    #if USING_GPIO
        // Initialize GPIO only on Linux
        initGPIOCommunication();
    #else
        printf("[SIMULATION] Skipping GPIO initialization\n");
        printf("[SIMULATION] Use this build for testing file operations only\n\n");
    #endif
}