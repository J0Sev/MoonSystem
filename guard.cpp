#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <vector>
#include <unordered_map>
using namespace std;

int gtou;
int utog;

unordered_map<int, string> fileBuffers;

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


bool checkWrite(const string& input) {
    return input.rfind("WRITE:", 0) == 0;
}




void executeOpen(string& input) {
    string args = input.substr(5);
    vector<string> vArgs = split(args, ',');

    cout << "fisrt arg" << vArgs[0] << endl;
    int fd;
    if (vArgs[1] == "r") {
        fd = open(vArgs[0].c_str(), O_RDONLY);
        fileBuffers[fd] = "";
    }
    else {
        fd = open(vArgs[0].c_str(), O_WRONLY);
        fileBuffers[fd] = "";
    }


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


    cout << fd << endl;
    cout << count << endl;


    char* buffer = new char[count + 10]; // Buffer to store read data
    ssize_t bytesRead = read(fd, buffer, count); // Read up to 99 bytes
    if (bytesRead == -1) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    buffer[bytesRead] = '\0'; // Null-terminate the string


    sendMessage(gtou, buffer);
}

void executeWrite(string& input) {
    vector<string> parts = split(input.substr(6), ',');
    int fd = stoi(parts[0]);
    string buf = parts[1];
    int count = stoi(parts[2]);




    char* buffer = new char[count + 10]; // Buffer to store read data
    ssize_t bytesWritten = write(fd, buffer, count); // Read up to 99 bytes
    if (bytesWritten == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }


    const char* fdMessage = to_string(bytesWritten).c_str();
    sendMessage(gtou, fdMessage);
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