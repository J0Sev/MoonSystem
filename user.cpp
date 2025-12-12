#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>


void sendMessage(int fd, char* message) {
    write(fd, message, strlen(message));
}


int main() {
    int gtol = open("/tmp/GtoL", O_RDONLY); // read from GtoL
    int ltog = open("/tmp/LtoG", O_WRONLY); // write to LtoG

    char inputFromG[100];
    char outputToG[100];

    ssize_t n = read(gtol, inputFromG, sizeof(inputFromG) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        std::cout << "Received: " << buffer << std::endl;
    }

    close(gtol);
    close(ltog);

}
