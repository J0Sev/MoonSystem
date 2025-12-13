#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <vector>
using namespace std;

int gtou;
int utog;


void fopen() {

}

void sendMessage(int fd, const char* message) {
    write(fd, message, strlen(message));
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

    cout << "args:::" << args << endl;

   

    FILE *fp = fopen(vArgs[0].c_str(), vArgs[1].c_str());


    int fd = fileno(fp);

    cout << fd << endl;

    const char* fdMessage = to_string(fd).c_str();

    printf("%s\n", fdMessage);
    sendMessage(gtou, fdMessage);
    
}

void executeSyscall(char* in) {
    string input(in);

    if (checkFopen(input))
        executeFopen(input);

   
}


int main() {
    gtou = open("/tmp/GtoU", O_WRONLY); // write to GtoL
    utog = open("/tmp/UtoG", O_RDONLY); // read from LtoG

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
