#include <iostream>
#include <string>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // For ShellExecute
#include <limits>

#pragma comment(lib, "Ws2_32.lib")

const std::string SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;

int main() {
    while (true) {
        std::cout << "Enter the path of the file to upload (or empty/quit to exit): ";
        std::string filePath;
        std::getline(std::cin, filePath);

        if (filePath.empty() || filePath == "quit") {
            std::cout << "Exiting...\n";
            break;
        }

        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open file." << std::endl;
            continue;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string fileContent(size, '\0');
        if (!file.read(&fileContent[0], size)) {
            std::cerr << "Failed to read file." << std::endl;
            continue;
        }
        file.close();
        std::cout << "[DEBUG] File size to upload: " << size << " bytes." << std::endl;

        // 从filePath中提取文件名（不包含路径）
        std::string filename = filePath;
        {
            size_t pos = filename.find_last_of("\\/");
            if (pos != std::string::npos) {
                filename = filename.substr(pos + 1);
            }
        }

        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed." << std::endl;
            continue;
        }

        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket." << std::endl;
            WSACleanup();
            continue;
        }

        sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP.c_str(), &serverAddr.sin_addr);

        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connect failed." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            continue;
        }

        // 使用X-Filename头传递文件名给服务器
        std::string requestHeader =
            "POST /upload HTTP/1.0\r\n"
            "Host: " + SERVER_IP + "\r\n"
            "Content-Type: application/octet-stream\r\n"
            "Content-Length: " + std::to_string(fileContent.size()) + "\r\n"
            "X-Filename: " + filename + "\r\n"
            "\r\n";

        std::string request = requestHeader + fileContent;
        int sent = send(clientSocket, request.c_str(), (int)request.size(), 0);
        std::cout << "[DEBUG] POST request sent bytes: " << sent << std::endl;

        if (sent == SOCKET_ERROR) {
            std::cerr << "Failed to send POST request. Error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            continue;
        }

        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "No response received or error. WSAGetLastError: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            continue;
        }
        buffer[bytesReceived] = '\0';
        std::string response(buffer);

        // 找到头结束位置
        size_t headerEnd = response.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            std::cerr << "No end of header found." << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            continue;
        }

        // 从响应头中解析Content-Length
        int contentLength = 0;
        {
            size_t clPos = response.find("Content-Length:");
            if (clPos != std::string::npos) {
                size_t startPos = clPos + 15;
                while (startPos < response.size() && (response[startPos] == ' ' || response[startPos] == '\t')) startPos++;
                size_t endPos = response.find("\r\n", startPos);
                if (endPos != std::string::npos) {
                    std::string lengthStr = response.substr(startPos, endPos - startPos);
                    try {
                        contentLength = std::stoi(lengthStr);
                    }
                    catch (...) {
                        std::cerr << "Failed to parse Content-Length: " << lengthStr << std::endl;
                    }
                }
            }
        }

        int alreadyReadBody = (int)response.size() - (int)(headerEnd + 4);
        while (alreadyReadBody < contentLength) {
            int toRead = contentLength - alreadyReadBody;
            int recvSize = (toRead < (int)sizeof(buffer) - 1) ? toRead : (int)sizeof(buffer) - 1;
            int ret = recv(clientSocket, buffer, recvSize, 0);
            if (ret <= 0) {
                std::cerr << "Connection lost or error during body receive. WSAGetLastError: " << WSAGetLastError() << std::endl;
                break;
            }
            buffer[ret] = '\0';
            response.append(buffer, ret);
            alreadyReadBody += ret;
        }

        closesocket(clientSocket);
        WSACleanup();

        std::cout << "Response: " << response << std::endl;

        // 查找URL字段
        size_t urlPos = response.find("\"url\":\"");
        if (urlPos == std::string::npos) {
            std::cerr << "No URL found in response." << std::endl;
            continue;
        }
        urlPos += 7;
        size_t endPos = response.find("\"", urlPos);
        if (endPos == std::string::npos) {
            std::cerr << "Invalid URL in response." << std::endl;
            continue;
        }
        std::string fileUrl = response.substr(urlPos, endPos - urlPos);
        std::cout << "Opening browser at URL: " << fileUrl << std::endl;

        // 打开浏览器
        ShellExecuteA(nullptr, "open", fileUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    system("pause");
    return 0;
}
