#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

const std::string SERVER_IP = "127.0.0.1"; // Server IP address
const int SERVER_PORT = 8080; // Server port

void sendPostRequest(const std::string& filePath) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // Clear structure before use
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Read file content
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Failed to read file." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }
    file.close();

    // Create HTTP POST request with dynamic file name
    std::ostringstream requestStream;
    requestStream << "POST /upload HTTP/1.1\r\n";
    requestStream << "Host: " << SERVER_IP << "\r\n";
    requestStream << "Content-Type: multipart/form-data; boundary=--boundary\r\n";
    requestStream << "Content-Length: " << size + 150 << "\r\n"; // Approximation for header size
    requestStream << "\r\n";
    requestStream << "--boundary\r\n";
    requestStream << "Content-Disposition: form-data; name=\"file\"; filename=\"" << filePath.substr(filePath.find_last_of("\\") + 1) << "\"\r\n";
    requestStream << "Content-Type: text/plain\r\n";
    requestStream << "\r\n";
    requestStream.write(buffer.data(), size);
    requestStream << "\r\n--boundary--\r\n";

    std::string request = requestStream.str();
    std::cout << "Request sent: \n" << request << std::endl; // Debugging request content

    send(clientSocket, request.c_str(), request.length(), 0);

    // Receive response from server
    char responseBuffer[1024];
    int bytesReceived = recv(clientSocket, responseBuffer, sizeof(responseBuffer) - 1, 0);
    if (bytesReceived > 0) {
        responseBuffer[bytesReceived] = '\0';
        std::cout << "Response received:\n" << responseBuffer << std::endl;
    }
    else {
        std::cerr << "No response received." << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    std::string filePath;
    std::cout << "Enter the path of the file to upload: ";
    std::getline(std::cin, filePath);
    sendPostRequest(filePath);
    system("pause");
    return 0;
}
