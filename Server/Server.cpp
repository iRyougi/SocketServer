#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <map>
#include <filesystem> // C++17

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;

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
    else if (ext == ".mp4") return "video/mp4";
    else if (ext == ".webm") return "video/webm";
    return "application/octet-stream";
}

int getContentLength(const std::string& request) {
    int contentLength = 0;
    size_t pos = request.find("Content-Length:");
    if (pos != std::string::npos) {
        size_t start = pos + 15;
        while (start < request.size() && (request[start] == ' ' || request[start] == '\t')) {
            start++;
        }
        size_t end = request.find("\r\n", start);
        if (end != std::string::npos) {
            std::string lengthStr = request.substr(start, end - start);
            try {
                contentLength = std::stoi(lengthStr);
            }
            catch (...) {
                std::cerr << "[ERROR] Failed to parse Content-Length: " << lengthStr << std::endl;
            }
        }
    }
    return contentLength;
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

    std::filesystem::create_directory("uploads");

    while (true) {
        std::cout << "[DEBUG] Waiting for client connection..." << std::endl;
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "[ERROR] Accept failed with error: " << WSAGetLastError() << std::endl;
            continue;
        }
        std::cout << "[DEBUG] Client connected." << std::endl;

        char headerBuffer[4096];
        int bytesReceived = recv(clientSocket, headerBuffer, sizeof(headerBuffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cerr << "[WARNING] Received no data or error. Error: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            continue;
        }
        headerBuffer[bytesReceived] = '\0';
        std::string request(headerBuffer);
        std::cout << "[DEBUG] Received request header:\n" << request << std::endl;

        bool isGet = (request.rfind("GET ", 0) == 0);
        bool isPost = (request.rfind("POST ", 0) == 0);

        if (isGet) {
            size_t start = request.find("GET ") + 4;
            size_t spacePos = request.find(' ', start);
            if (spacePos == std::string::npos) {
                std::cerr << "[DEBUG] Malformed GET request line, returning 404." << std::endl;
                std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
                send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
                closesocket(clientSocket);
                continue;
            }
            std::string path = request.substr(start, spacePos - start);

            if (path == "/13-knight" || path == "/13-knight/")
                path = "/13-knight/index.html";

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

            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            std::cout << "[DEBUG] File opened and read successfully. Size: " << content.size() << " bytes." << std::endl;

            std::string contentType = getContentType(path);
            std::string responseHeader =
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: " + contentType + "\r\n"
                "Content-Length: " + std::to_string(content.size()) + "\r\n"
                "\r\n";

            send(clientSocket, responseHeader.c_str(), (int)responseHeader.size(), 0);
            send(clientSocket, content.c_str(), (int)content.size(), 0);

            closesocket(clientSocket);
            std::cout << "[DEBUG] Client connection closed." << std::endl;
        }
        else if (isPost) {
            size_t start = request.find("POST ") + 5;
            size_t spacePos = request.find(' ', start);
            if (spacePos == std::string::npos) {
                std::cerr << "[DEBUG] Malformed POST request line, returning 404." << std::endl;
                std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
                send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
                closesocket(clientSocket);
                continue;
            }
            std::string reqPath = request.substr(start, spacePos - start);
            if (reqPath != "/upload") {
                std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
                send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
                closesocket(clientSocket);
                continue;
            }

            int contentLength = getContentLength(request);
            std::cout << "[DEBUG] POST /upload with Content-Length: " << contentLength << std::endl;

            size_t headerEndPos = request.find("\r\n\r\n");
            if (headerEndPos == std::string::npos) {
                std::cerr << "[ERROR] No end of header found." << std::endl;
                closesocket(clientSocket);
                continue;
            }

            std::string body;
            body.append(request.begin() + headerEndPos + 4, request.end());

            // 如果body还未达到contentLength，继续接收
            while ((int)body.size() < contentLength) {
                char tempBuf[4096];
                int ret = recv(clientSocket, tempBuf, sizeof(tempBuf), 0);
                if (ret <= 0) {
                    std::cerr << "[ERROR] Connection lost while receiving POST body." << std::endl;
                    closesocket(clientSocket);
                    // 此处continue无法让外部逻辑执行完整，所以加个break退出循环
                    break;
                }
                body.append(tempBuf, ret);
            }

            std::cout << "[DEBUG] Received POST body size: " << body.size() << " bytes." << std::endl;

            std::string uploadPath = "uploads/uploaded_file";
            {
                std::ofstream outFile(uploadPath, std::ios::binary);
                if (!outFile.is_open()) {
                    std::cerr << "[ERROR] Cannot open " << uploadPath << " for writing." << std::endl;
                }
                outFile.write(body.data(), body.size());
            }

            std::string fileUrl = "http://127.0.0.1:8080/" + uploadPath;
            std::string jsonResponse = "{\"url\":\"" + fileUrl + "\"}";
            std::cout << "[DEBUG] JSON Response: " << jsonResponse << std::endl;

            std::string responseHeader =
                "HTTP/1.0 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(jsonResponse.size()) + "\r\n"
                "\r\n";

            int headerSent = send(clientSocket, responseHeader.c_str(), (int)responseHeader.size(), 0);
            int bodySent = send(clientSocket, jsonResponse.c_str(), (int)jsonResponse.size(), 0);

            std::cout << "[DEBUG] JSON headerSent: " << headerSent << ", bodySent: " << bodySent << std::endl;

            closesocket(clientSocket);
            std::cout << "[DEBUG] Client connection closed. Uploaded file saved as " << uploadPath << std::endl;
        }
        else {
            std::cerr << "[DEBUG] Unsupported request method. Returning 404." << std::endl;
            std::string notFoundResponse = "HTTP/1.0 404 Not Found\r\nContent-Length:0\r\n\r\n";
            send(clientSocket, notFoundResponse.c_str(), (int)notFoundResponse.size(), 0);
            closesocket(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
