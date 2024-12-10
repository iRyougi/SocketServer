// GETClient.cpp

#include <iostream>
#include <windows.h> // For ShellExecute

int main() {
    std::cout << "是否打开网页？（是请输入Y，输入其他内容退出程序）" << std::endl;
    char choice;
    std::cin >> choice;
    if (choice == 'Y' || choice == 'y') {
        // 直接让浏览器打开服务器上的页面
        std::string url = "http://127.0.0.1:8080/13-knight/index.html";
        ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    else {
        std::cout << "程序退出。" << std::endl;
    }
    system("pause");
    return 0;
}
