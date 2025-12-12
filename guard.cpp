#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

int main() {
    int gtol = open("/tmp/GtoL", O_WRONLY); // write to GtoL
    int ltog = open("/tmp/LtoG", O_RDONLY); // read from LtoG

    const char* msg = "hiii";
    write(gtol, msg, strlen(msg));

    close(gtol);
    close(ltog);

}
