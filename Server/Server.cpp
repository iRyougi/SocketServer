#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <map>

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;

// 根据文件扩展名返回Content-Type
std::string getContentType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return "application/octet-stream";

    std::string ext = path.substr(dotPos);
    if (ext == ".html" || ext == ".htm") return "text/html";
    else if (ext == ".css") return "text/css";
    else if (ext == ".js") return "application/javascript";
    else if (ext == ".png") return "image/png";
    else if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    else if (ext == ".gif") return "image/gif";
    else if (ext == ".ico") return "image/x-icon";
    else if (ext == ".svg") return "image/svg+xml";
    return "application/octet-stream";
}

int main() {
    std::cout << "[DEBUG] Starting server initialization..." << std::endl;

    WSADATA wsaData;
    int wsaInit = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaInit != 0) {
        std::cerr << "[ERROR] WSAStartup failed with error: " << wsaInit << std::endl;
        return 1;
    }
    std::cout << "[DEBUG] WSAStartup successful." << std::endl;

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[ERROR] Failed to create socket. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "[DEBUG] Socket created successfully." << std::endl;

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Bind failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "[DEBUG] Bind successful on port " << PORT << "." << std::endl;

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Listen failed with error: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "[DEBUG] Server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        std::cout << "[DEBUG] Waiting for client connection..." << std::endl;
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "[ERROR] Accept failed with error: " << WSAGetLastError() << std::endl;
            continue; // 尝试接受下一个连接
        }
        std::cout << "[DEBUG] Client connected." << std::endl;

        char buffer[2048];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "[WARNING] Received no data or error occurred. Error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            continue;
        }
        buffer[bytesReceived] = '\0';

        std::string request(buffer);
        std::cout << "[DEBUG] Received request: " << request << std::endl;

        // 解析GET请求行
        size_t pos = request.find("GET ");
        if (pos == std::string::npos) {
            std::cerr << "[DEBUG] Not a GET request, returning 404." << std::endl;
            std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
            send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
            closesocket(clientSocket);
            continue;
        }

        size_t start = pos + 4; // 跳过 "GET "
        size_t spacePos = request.find(' ', start);
        if (spacePos == std::string::npos) {
            std::cerr << "[DEBUG] Malformed request line, returning 404." << std::endl;
            std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
            send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
            closesocket(clientSocket);
            continue;
        }
        std::string path = request.substr(start, spacePos - start);

        // 若请求路径是 /13-knight 或 /13-knight/
        // 则重定向到 index.html
        if (path == "/13-knight" || path == "/13-knight/") {
            path = "/13-knight/index.html";
        }

        // 去掉路径前导 '/'
        if (!path.empty() && path.front() == '/')
            path.erase(0, 1);

        std::cout << "[DEBUG] Mapped request to file: " << path << std::endl;

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ERROR] Failed to open file: " << path << ". Returning 404." << std::endl;
            std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
            send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
            closesocket(clientSocket);
            continue;
        }

        // 读取文件内容
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        std::cout << "[DEBUG] File opened and read successfully. Size: " << content.size() << " bytes." << std::endl;

        // 确定Content-Type
        std::string contentType = getContentType(path);

        std::string responseHeader =
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: " + contentType + "\r\n"
            "Content-Length: " + std::to_string(content.size()) + "\r\n"
            "\r\n";

        int headerSent = send(clientSocket, responseHeader.c_str(), (int)responseHeader.size(), 0);
        if (headerSent == SOCKET_ERROR) {
            std::cerr << "[ERROR] Failed to send response header. Error: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "[DEBUG] Response header sent. Length: " << responseHeader.size() << " bytes." << std::endl;
        }

        int contentSent = send(clientSocket, content.c_str(), (int)content.size(), 0);
        if (contentSent == SOCKET_ERROR) {
            std::cerr << "[ERROR] Failed to send response content. Error: " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "[DEBUG] Response content sent. Length: " << content.size() << " bytes." << std::endl;
        }

        closesocket(clientSocket);
        std::cout << "[DEBUG] Client connection closed." << std::endl;
    }

    // 一般不会到达这里
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
