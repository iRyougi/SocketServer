#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <fstream>
#include <vector>
#include <sstream>
#include <windows.h> // For ShellExecute

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;
const std::string SERVER_IP = "127.0.0.1";

void sendGetRequestToServer(const std::string& path) {
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
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed." << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Build GET request
    std::string request = "GET /" + path + " HTTP/1.0\r\n\r\n";
    send(clientSocket, request.c_str(), request.length(), 0);

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Response received:\n" << buffer << std::endl;
    }
    else {
        std::cerr << "No response received." << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}

void listFiles() {
    // Send GET request to fetch the list of files from the server
    sendGetRequestToServer("list");
}

void openURLInBrowser(const std::string& url) {
    // Open the URL using the default browser
    ShellExecute(0, 0, url.c_str(), 0, 0, SW_SHOWNORMAL);
}

int main() {
    std::string input;
    std::cout << "Enter the file path: ";
    std::cin >> input;
    sendGetRequestToServer(input);  // Otherwise, send GET request for a specific file or resource
    std::cout << "Do you want to see it? (Y/N): ";
    char choice;
    std::cin >> choice;

    if (choice == 'Y' || choice == 'y') {
        std::string localPath = "D:/VSCodeCPP/WWW/project/" + input;
        // Replace backslashes with forward slashes and add file:// protocol
        std::string url = localPath;
        openURLInBrowser(url);
    }
    else if (choice == 'N' || choice == 'n') {
        std::string url;
        std::cout << "Enter the URL to open: ";
        std::cin.ignore();  // Ignore leftover newline character
        std::getline(std::cin, url);  // Read the full URL
        if (url.find("http://") != 0 && url.find("https://") != 0) {
            url = "http://" + url;  // Ensure the URL starts with http://
        }
        openURLInBrowser(url);
    }
    else {
        std::cout << "Invalid choice." << std::endl;
    }
    system("pause");
    return 0;
}
