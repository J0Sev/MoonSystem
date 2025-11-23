#include <stdio.h>
#include <stdlib.h>

#define FOPEN_SIGNAL	'0'
#define NEXT_ARG		'1'


struct FILE {

};




void sendByteToGM(char byte) {
	printf("%c", byte);
}

void sendStringToGM(char* s) {
	printf("%s", s);
}

void awaitSignalFromGM(char b) {
	//hang until GM sends b signal
	char buffer[2];
	fgets(buffer, 2, stdin);

	if (buffer[0] == b)
		//GM properly sent the signal
		return;

	//GM failed to process the syscall
	exit(1);

}


//read numBytes bytes from GM into a buffer and return the pointer to the buffer
char* getStreamFromGM(uint numBytes) {
	char *buffer = malloc(numBytes * sizeof(char)); // array of 10 ints
	fgets(buffer, numBytes, stdin);

	return buffer;
}


FILE* fopen(const char *filename, const char *mode) {
	//send the fopen signal to GM
	sendByteToGM(FOPEN_SIGNAL);

	//wait for GM to request the next argument to fopen
	awaitSignalFromGM(NEXT_ARG);

	//send filename to GM
	sendStringToGM(filename);

	char* buffer[8];
	sendByteToGM(mode);

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