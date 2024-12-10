#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <fstream>
#include <vector>
#include <filesystem>
#include <mutex> // std::mutex fileMutex; // Mutex for thread-safe operations on shared resources
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include <sstream>


#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;
const std::string WEB_ROOT = "D:\SoPHomore\WORld wIde WEb\WWW\WWW\project"; // Your web root directory here
std::mutex fileMutex; // Mutex for thread-safe operations on shared resources

void sendResponse(SOCKET clientSocket, const std::string& response) {
    int result = send(clientSocket, response.c_str(), response.length(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send response" << std::endl;
    }
}

std::string readRequestBody(SOCKET clientSocket, int contentLength) {
    std::vector<char> buffer(contentLength);
    recv(clientSocket, buffer.data(), contentLength, 0);
    return std::string(buffer.begin(), buffer.end());
}

void listFiles(const std::string& directoryPath, std::string& fileListMessage) {
    try {
        if (std::filesystem::exists(directoryPath) && std::filesystem::is_directory(directoryPath)) {
            for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
                if (std::filesystem::is_regular_file(entry.status())) {
                    fileListMessage += entry.path().filename().string() + "\r\r\r\r";
                }
            }
        }
        else {
            std::cerr << "The specified path is not a valid directory." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error while listing files: " << e.what() << std::endl;
    }
}

void handleClient(SOCKET clientSocket) {
    char buffer[8192];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "No bytes received or connection closed." << std::endl;
        closesocket(clientSocket);
        return;
    }
    std::cout << "Request received:\n" << std::string(buffer, bytesReceived) << std::endl;

    std::istringstream requestStream(std::string(buffer, bytesReceived));
    std::string method, path, protocol;
    requestStream >> method >> path >> protocol;

    if (method == "POST" && path == "/upload") {
        std::string headerLine;
        int contentLength = 0;
        //std::istringstream requestStream(std::string(buffer, bytesReceived)); // 确保 buffer 初始化正确

        while (std::getline(requestStream, headerLine) && headerLine != "\r") {
            if (headerLine.find("Content-Length:") != std::string::npos) {
                contentLength = std::stoi(headerLine.substr(15));
            }
        }

        std::string postData = readRequestBody(clientSocket, contentLength);
        size_t boundaryPos = postData.find("boundary=");
        if (boundaryPos != std::string::npos) {
            std::string boundary = "--" + postData.substr(boundaryPos + 9);
            size_t start = postData.find("\r\n\r\n", boundaryPos) + 4;
            size_t end = postData.find(boundary + "--", start);
            std::string fileData = postData.substr(start, end - start);
            size_t filenamePos = fileData.find("filename=\"");
            if (filenamePos != std::string::npos) {
                size_t filenameEnd = fileData.find("\"", filenamePos + 10);
                std::string filename = fileData.substr(filenamePos + 10, filenameEnd - (filenamePos + 10));
                std::ofstream uploadedFile(WEB_ROOT + std::string("\\") + filename, std::ios::binary);
                uploadedFile.write(fileData.c_str(), fileData.size());
                uploadedFile.close();
                std::cout << "File content:\n" << fileData << std::endl;
                sendResponse(clientSocket, "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n\r\nFile uploaded successfully.");
                std::cout << "yes" << std::endl;
            }
            else {
                sendResponse(clientSocket, "HTTP/1.0 400 Bad Request\r\n\r\nError: Failed to parse POST data.");
                std::cout << "wrong" << std::endl;
            }
        }
        else {
            sendResponse(clientSocket, "HTTP/1.0 400 Bad Request\r\n\r\nError: Failed to parse POST data.");
            std::cout << "no" << std::endl;
        }
        std::cout << "finished" << std::endl;
    }
    else if (method == "GET") {
        std::string filePath = WEB_ROOT + path;
        if (path == "/") {
            filePath += "index.html"; // Default file to serve when accessing the root URL
        }
        std::ifstream requestedFile(filePath);
        if (requestedFile) {
            std::ostringstream ss;
            ss << requestedFile.rdbuf();
            std::string fileContent = ss.str();
            std::string response = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(fileContent.size()) + "\r\n\r\n" + fileContent;
            sendResponse(clientSocket, response);
        }
        else {
            std::string notFoundMessage = "404 - Not Found";
            std::string fileListMessage;
            listFiles(WEB_ROOT, fileListMessage);
            std::string response = "HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(notFoundMessage.size() + fileListMessage.size()) + "\r" + notFoundMessage + ". The file you requested was not found. Here are the available files in /project: " + fileListMessage;
            sendResponse(clientSocket, response);
        }
    }
    else {
        sendResponse(clientSocket, "HTTP/1.0 405 Method Not Allowed\r\n\r\n");
    }
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }
        std::thread(handleClient, clientSocket).detach(); // Create a new thread for each client connection and detach it
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
