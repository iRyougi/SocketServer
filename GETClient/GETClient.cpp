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
        std::cout << "�����������URL (����: 127.0.0.1 ������URL, �س���'quit'�˳�): ";
        std::string input;
        std::getline(std::cin, input);

        // ���û�����Ϊ�ջ�����"quit"���˳�ѭ��
        if (input.empty() || input == "quit") {
            std::cout << "���������" << std::endl;
            break;
        }

        // ȥ����β�ո�
        while (!input.empty() && (input.front() == ' ' || input.front() == '\t')) input.erase(input.begin());
        while (!input.empty() && (input.back() == ' ' || input.back() == '\t')) input.pop_back();

        // ����û�û������������ "127.0.0.1"��Ĭ�Ϸ��� index.html
        if (input.empty() || input == LOCAL_HOST) {
            input = LOCAL_HOST + "/13-knight/index.html";
        }

        bool isLocal = false;
        if (input.find(LOCAL_HOST) == 0) {
            // ��127.0.0.1��ͷ����Ϊ���ط���������
            isLocal = true;
        }

        if (isLocal) {
            // ��ȡ·��
            std::string path = input.substr(LOCAL_HOST.size());
            if (path.empty()) {
                path = "/index.html";
            }
            else if (path.front() != '/') {
                path = "/" + path;
            }

            // ��ʼ��WinSock
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

                // ѯ���Ƿ���������д�
                std::cout << "�Ƿ���������д򿪣�(Y/N): ";
                char choice;
                std::cin >> choice;
                std::cin.ignore(1024, '\n');  // ������з�
                if (choice == 'Y' || choice == 'y') {
                    std::string url = "http://" + LOCAL_HOST + ":" + std::to_string(PORT) + path;
                    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
                else {
                    std::cout << "����������д򿪡�" << std::endl;
                }
            }

        }
        else {
            // �Ǳ��ط�����URL��ֱ�����������
            std::string url = input;
            // ����httpͷ����
            if (url.find("http://") != 0 && url.find("https://") != 0) {
                url = "http://" + url;
            }
            ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }

    system("pause");
    return 0;
}
