#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <vector>
using namespace std;


void fopen() {

}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}


bool checkFopen(string& input) {

    cout << input.rfind("FOPEN:", 0) << endl;
    if (input.rfind("FOPEN:", 0) == 0)
        return true;
    return false;
}

void executeFopen(string& input) {
    string args = input.substr(6);
    vector<string> vArgs = split(args, ',');

    cout << args << endl;
    
}

void executeSyscall(char* in) {
    string input(in);

    if (checkFopen(input))
        executeFopen(input);

   
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
            executeSyscall(inputFromU);
        }
    }

    close(gtou);
    close(utog);

}
