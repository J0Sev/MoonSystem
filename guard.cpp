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


bool checkOpen(string& input) {
    cout << input.rfind("OPEN:", 0) << endl;
    if (input.rfind("OPEN:", 0) == 0)
        return true;
    return false;
}




bool checkClose(string& input) {
    if (input.rfind("CLOSE:", 0) == 0)
        return true;
    return false;
}



bool checkRead(const string& input) {
    return input.rfind("READ:", 0) == 0;
}






void executeOpen(string& input) {
    string args = input.substr(5);
    vector<string> vArgs = split(args, ',');

    cout << "fisrt arg" << vArgs[0] << endl;
    int fd;
    if (vArgs[1] == "r")
        fd = open(vArgs[0].c_str(), O_RDONLY);
    else
        fd = open(vArgs[0].c_str(), O_WRONLY);


    cout << "FD: " << fd << endl;
    const char* fdMessage = to_string(fd).c_str();

    printf("%s\n", fdMessage);
    sendMessage(gtou, fdMessage);
    
}


void executeClose(string& input) {
    string fileDescriptor = input.substr(6);

    int fd = stoi(fileDescriptor);


   

    cout << "closed: " << close(fd) << endl;
    
}



void executeRead(string& input) {
    vector<string> parts = split(input.substr(5), ',');
    int fd = stoi(parts[0]);
    int count = stoi(parts[1]);

    string data = "";
    if (fileBuffers.count(fd)) {
        data = fileBuffers[fd].substr(0, count);
        fileBuffers[fd] = fileBuffers[fd].substr(count); // consume data
    }

    sendMessage(gtou, data.c_str());
}




void executeSyscall(char* in) {
    string input(in);

    if (checkOpen(input))
        executeOpen(input);
    else if (checkClose(input))
        executeClose(input);
    else if (checkRead(input))
        executeRead(input);
    else if (checkWrite(input))
        executeWrite(input);
   
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
