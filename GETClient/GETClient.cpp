// GETClient.cpp

#include <iostream>
#include <windows.h> // For ShellExecute

int main() {
    std::cout << "�Ƿ����ҳ������������Y���������������˳�����" << std::endl;
    char choice;
    std::cin >> choice;
    if (choice == 'Y' || choice == 'y') {
        // ֱ����������򿪷������ϵ�ҳ��
        std::string url = "http://127.0.0.1:8080/13-knight/index.html";
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    else {
        std::cout << "�����˳���" << std::endl;
    }
    system("pause");
    return 0;
}
