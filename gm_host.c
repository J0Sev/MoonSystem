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