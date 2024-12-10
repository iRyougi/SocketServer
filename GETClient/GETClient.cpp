#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <limits>
#include <windows.h> // For ShellExecute

#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;
const std::string LOCAL_HOST = "127.0.0.1";

int main() {
    while (true) {
        std::cout << "请输入服务器URL (例如: 127.0.0.1 或其它URL, 回车或'quit'退出): ";
        std::string input;
        std::getline(std::cin, input);

        // 若用户输入为空或输入"quit"则退出循环
        if (input.empty() || input == "quit") {
            std::cout << "程序结束。" << std::endl;
            break;
        }

        // 去掉首尾空格
        while (!input.empty() && (input.front() == ' ' || input.front() == '\t')) input.erase(input.begin());
        while (!input.empty() && (input.back() == ' ' || input.back() == '\t')) input.pop_back();

        // 如果用户没有输入或仅输入 "127.0.0.1"，默认访问 index.html
        if (input.empty() || input == LOCAL_HOST) {
            input = LOCAL_HOST + "/13-knight/index.html";
        }

        bool isLocal = false;
        if (input.find(LOCAL_HOST) == 0) {
            // 以127.0.0.1开头，认为本地服务器请求
            isLocal = true;
        }

        if (isLocal) {
            // 提取路径
            std::string path = input.substr(LOCAL_HOST.size());
            if (path.empty()) {
                path = "/index.html";
            }
            else if (path.front() != '/') {
                path = "/" + path;
            }

            // 初始化WinSock
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
            serverAddr.sin_port = htons(PORT);
            inet_pton(AF_INET, LOCAL_HOST.c_str(), &serverAddr.sin_addr);

            if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cerr << "Connect failed." << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                continue;
            }

            std::string request = "GET " + path + " HTTP/1.0\r\nHost: " + LOCAL_HOST + "\r\n\r\n";
            send(clientSocket, request.c_str(), (int)request.size(), 0);

            char buffer[4096];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            std::string response;
            while (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                response.append(buffer, bytesReceived);
                bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            }

            closesocket(clientSocket);
            WSACleanup();

            if (response.empty()) {
                std::cerr << "No response received." << std::endl;
            }
            else {
                std::cout << "Response received:\n" << response << std::endl;

                // 询问是否在浏览器中打开
                std::cout << "是否在浏览器中打开？(Y/N): ";
                char choice;
                std::cin >> choice;
                std::cin.ignore(1024, '\n');  // 清除换行符
                if (choice == 'Y' || choice == 'y') {
                    std::string url = "http://" + LOCAL_HOST + ":" + std::to_string(PORT) + path;
                    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
                else {
                    std::cout << "不在浏览器中打开。" << std::endl;
                }
            }

        }
        else {
            // 非本地服务器URL，直接用浏览器打开
            std::string url = input;
            // 若无http头则补齐
            if (url.find("http://") != 0 && url.find("https://") != 0) {
                url = "http://" + url;
            }
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    system("pause");
    return 0;
}
