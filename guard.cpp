#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>


void fopen() {

}



int main() {
    int gtou = open("/tmp/GtoU", O_WRONLY); // write to GtoL
    int utog = open("/tmp/UtoG", O_RDONLY); // read from LtoG

    const char* msg = "hiii";
    write(gtou, msg, strlen(msg));

    char outputToU[100];
    char inputFromU[100];

    while (true) {
        ssize_t n = read(utog, inputFromU, sizeof(inputFromU) - 1);
        if (n > 0) {
            inputFromU[n] = '\0';
            std::cout << "Received: " << inputFromU << std::endl;
        }
    }

    close(gtou);
    close(utog);

}
