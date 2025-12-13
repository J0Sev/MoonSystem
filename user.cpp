#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string> 
using namespace std;

int gtou; // read from GtoL
int utog;

char inputFromG[100];
char outputToG[100];

void sendMessage(int fd, char* message) {
    write(fd, message, strlen(message));
}



void readMessageFromG() {
    ssize_t n = read(gtou, inputFromG, sizeof(inputFromG) - 1);
}




int myFopen(string filename, string mode) {
    string mString = "FOPEN:" + filename + "," + mode;

    char* m = new char[mString.size() + 1];
    std::strcpy(m, mString.c_str());
    sendMessage(utog, m);

    readMessageFromG();

    printf("%s", inputFromG);

    string inputFromGString(inputFromG);

    return stoi(inputFromGString);

}




int main() {
    gtou = open("/tmp/GtoU", O_RDONLY); // read from GtoL
    utog = open("/tmp/UtoG", O_WRONLY); // write to LtoG

 

    myFopen("file.txt", "r");

    close(gtou);
    close(utog);

}
