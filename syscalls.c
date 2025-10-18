#define FOPEN_SIGNAL	0
#define NEXT_ARG		1
#include "syscalls.h"

void sendSignalToGM(char b) {

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

void awaitSignalFromGM(char b) {
	//hang until GM sends b signal
}

char* getStreamFromGM(uint numBytes) {
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