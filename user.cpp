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




int myOpen(string filename, string mode) {
    string mString = "OPEN:" + filename + "," + mode;

    char* m = new char[mString.size() + 1];
    std::strcpy(m, mString.c_str());
    sendMessage(utog, m);

    readMessageFromG();

    printf("%s", inputFromG);

    string inputFromGString(inputFromG);

    return stoi(inputFromGString);

}


void myClose(int fd) {
    string mString = "CLOSE:" + to_string(fd);

    char* m = new char[mString.size() + 1];
    std::strcpy(m, mString.c_str());
    sendMessage(utog, m);

    readMessageFromG();

    printf("%s", inputFromG);

}


int myRead(int fd, char* buffer, int count) {
    string mString = "READ:" + to_string(fd) + "," + to_string(count);

    char* m = new char[mString.size() + 1];
    strcpy(m, mString.c_str());
    sendMessage(utog, m);
    delete[] m;

    readMessageFromG();
    strncpy(buffer, inputFromG, count);
    buffer[count] = '\0'; // ensure null-terminated

    return strlen(buffer);
}



int myWrite(int fd, char* buffer, int count) {
    string bufferString(buffer);
    string mString = "WRITE:" + to_string(fd) + "," + bufferString + "," + to_string(count);
    return 1;
}




int mymain() {
    int fd = myOpen("file.txt", "r");
     
    char readBuffer[50];
    int bytesRead = myRead(fd, readBuffer, 50);
    printf("Read %d bytes: %s\n", bytesRead, readBuffer);



    myClose(fd);

    return 0;
}



int main() {
    gtou = open("/tmp/GtoU", O_RDONLY); // read from GtoL
    utog = open("/tmp/UtoG", O_WRONLY); // write to LtoG

 
    int r = mymain();

    close(gtou);
    close(utog);

}