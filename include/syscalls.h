#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_OPEN_FILES 16
#define FILENAME_MAX_LENGTH 32
#define SECTOR_SIZE 512

#define MODE_READ 0x01
#define MODE_WRITE 0X02
#define MODE_APPEND 0X04

#endif