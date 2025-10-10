#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_OPEN_FILES 16
#define FILENAME_MAX_LEN 32
#define SECTOR_SIZE 512

#define MODE_READ 0x01
#define MODE_WRITE 0X02
#define MODE_APPEND 0X04

//Controls where fseek() operates from
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

//File structure necesarry for standard system call commands
typedef struct {
    uint8_t is_open;
    uint8_t mode;
    uint8_t current_cluster;
    uint8_t file_size;
    uint32_t position;
    uint8_t buffer[SECTOR_SIZE];
    uint32_t buffer_cluster;
    uint8_t buffer_dirty;
    char filename[FILENAME_MAX_LEN];
} FILE;

//Function prototypes
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
int fseek(FILE* stream, long offset, int origin);
long ftell(FILE* stream);
int feof(FILE* stream);
void rewind(FILE* stream);

//Lower level functions required for main functions
int fs_init(void);
uint32_t fs_find_file(const char* filename);
uint32_t fs_create_file(const char* filename);
int fs_read_cluster(uint32_t cluster, uint8_t* buffer);
int fs_write_cluster(uint32_t cluster, const uint8_t buffer);
uint32_t fs_get_next_cluster(uint32_t cluster);

#endif