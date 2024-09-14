#include "pch.h"
#include "AdbClient.h"
#include "CommandExecution.h"
#include <fstream>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <tlhelp32.h>
#pragma comment(lib, "ws2_32.lib")



AdbClient::AdbClient()
{

}

AdbClient::~AdbClient()
{
    cleanupWsa();
}

// 终端命令执行的方式（powershell）异步执行adb命令
std::future<CommandExecution::CommandResult> AdbClient::run_async(const CString& command)
{
    return CommandExecution::ExecCommandAsync(_T("adb.exe ") + command);
}

// 终端命令执行的方式（powershell）阻塞的执行adb命令
CString AdbClient::run(const CString& command)
{
    // ??? CommandExecution ???????????????
    auto result = CommandExecution::ExecCommand(_T("adb.exe ") + command);
    if (result.result != 0) {
        return result.errorMsg;
    }
    return result.successMsg;
}

// 使用 CommandExecution 类的同步执行命令方法
std::vector<CString> AdbClient::DetectDevices()
{
    std::vector<CString> m_devices;

    CString output = AdbClient::run(_T("devices"));

    int pos = 0;
    CString line = output.Tokenize(_T("\r\n"), pos);
    while (!line.IsEmpty())
    {
        int tabPos = line.Find(_T('\t'));
        if (tabPos != -1)
        {
            CString deviceId = line.Left(tabPos);
            m_devices.push_back(deviceId);
        }
        line = output.Tokenize(_T("\r\n"), pos);
    }
    return m_devices;
}

// 通过终端命令执行异步安装多个apk，不阻塞，不关心是否成功
bool AdbClient::InstallAPK(const CString& deviceId, std::vector<CString> apks, bool force)
{
    if (deviceId.IsEmpty()) {
        return false;
    }

    CString command;
    command.Format(_T("-s %s install-multi-package"), deviceId.GetString());

    if (force) command.Append(_T(" -r -d -t")); // 强制安装，允许覆盖、降级、测试签
    bool hasValidApk = false;
    for (auto apk : apks)
    {
        std::ifstream file(apk);
        if (!file.good()) continue;
        // adb????.apk??.apex
        if (apk.Right(4).CompareNoCase(_T(".apk")) != 0 && apk.Right(5).CompareNoCase(_T(".apex")) != 0)  continue;
        command.AppendFormat(_T(" \"%s\""), apk.GetBuffer());
        hasValidApk = true;
    }
    // 确保至少有一个有效的 APK 文件，可以排除掉选择多个文件包含了apk但不都是apk的情况
    if (!hasValidApk)  return false;

    // 异步执行命令
    run_async(command);

    return true;
}

bool AdbClient::initializeWsa() {
    if (!wsaInitialized) {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return false;
        }
        wsaInitialized = true;
    }
    return true;
}

void AdbClient::cleanupWsa() {
    if (wsaInitialized) {
        WSACleanup();
        wsaInitialized = false;
    }
}

SOCKET AdbClient::createAndConnectSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation error" << std::endl;
        return INVALID_SOCKET;
    }

    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ADB_PORT);
    inet_pton(AF_INET, ADB_HOST.c_str(), &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection Failed" << std::endl;
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

AdbClient::AdbResponse AdbClient::sendCommand(const std::string& command) {
    if (!initializeWsa()) {
        return { "ERRO", "WSA initialization failed" };
    }

    SOCKET sock = createAndConnectSocket();
    if (sock == INVALID_SOCKET) {
        cleanupWsa();
        return { "ERRO", "Socket creation or connection failed" };
    }

    char lengthStr[5];
    std::snprintf(lengthStr, sizeof(lengthStr), "%04x", static_cast<int>(command.size()));
    std::string msg = std::string(lengthStr) + command;
    send(sock, msg.c_str(), msg.length(), 0);

    char response[5] = { 0 };
    if (recv(sock, response, 4, 0) <= 0) {
        closesocket(sock);
        cleanupWsa();
        return { "ERRO", "Failed to receive response" };
    }

    AdbResponse result;
    if (strncmp(response, "OKAY", 4) == 0) {
        result = receiveData(sock);
    }
    else if (strncmp(response, "FAIL", 4) == 0) {
        result = receiveErrorMessage(sock);
    }
    else {
        result = { "ERRO", "Unexpected response: " + std::string(response, 4) };
    }

    closesocket(sock);
    cleanupWsa();
    return result;
}

AdbClient::AdbResponse AdbClient::receiveData(SOCKET sock) {
    char lengthBuffer[5] = { 0 };
    if (recv(sock, lengthBuffer, 4, 0) <= 0) {
        return { "ERRO", "Failed to receive data length" };
    }

    int dataLength = std::stoi(std::string(lengthBuffer), nullptr, 16);
    std::string data(dataLength, '\0');
    int received = recv(sock, &data[0], dataLength, 0);
    if (received > 0) {
        return { "OKAY", data };
    }
    else {
        return { "ERRO", "Failed to receive complete data" };
    }
}

AdbClient::AdbResponse AdbClient::receiveErrorMessage(SOCKET sock) {
    char lengthBuffer[5] = { 0 };
    if (recv(sock, lengthBuffer, 4, 0) <= 0) {
        return { "ERRO", "Failed to receive error length" };
    }

    int errorLength = std::stoi(std::string(lengthBuffer), nullptr, 16);
    std::string errorMessage(errorLength, '\0');
    int received = recv(sock, &errorMessage[0], errorLength, 0);
    if (received > 0) {
        return { "FAIL", errorMessage };
    }
    else {
        return { "ERRO", "Failed to receive complete error message" };
    }
}

bool AdbClient::isProcessRunning(const TCHAR* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return false;
    }

    do {
        if (_tcsicmp(pe32.szExeFile, processName) == 0) {  // Use _tcsicmp for case-insensitive comparison
            CloseHandle(hSnapshot);
            return true;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
    return false;
}

bool AdbClient::isAdbServerRunning() {
    return isProcessRunning(_T("adb.exe"));
}

bool AdbClient::startAdbServer() {
    if (isAdbServerRunning()) {
        return true;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CommandExecution::ExecCommand(_T("adb.exe start-server")).result != 0) {
        std::cerr << "Failed to start ADB server" << std::endl;
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return isAdbServerRunning();
}

bool AdbClient::stopAdbServer() {
    AdbResponse response = sendCommand("host:kill");
    return response.status == "OKAY";
}

std::vector<std::string> AdbClient::getConnectedDevices() {
    AdbResponse response = sendCommand("host:devices");
    if (response.status != "OKAY") {
        return {};
    }

    std::vector<std::string> devices;

    // 处理整个响应字符串
    size_t start = 0;
    size_t end = response.data.find("\n");

    // 遍历每一行
    while (end != std::string::npos) {
        std::string device = response.data.substr(start, end - start);
        size_t tab_pos = device.find("\t");

        // 检查是否找到设备，并且设备状态是否为 "device"
        if (tab_pos != std::string::npos) {
            std::string device_serial = device.substr(0, tab_pos);
            std::string device_status = device.substr(tab_pos + 1);

            // 检查设备状态是否为有效状态（如 "device"）
            if (device_status.find("device") != std::string::npos) {
                devices.push_back(device_serial);
            }
        }

        // ??????????
        start = end + 1;
        end = response.data.find("\n", start);
    }

    // 移动到下一行
    std::string last_device = response.data.substr(start);
    size_t tab_pos = last_device.find("\t");
    if (tab_pos != std::string::npos) {
        std::string device_serial = last_device.substr(0, tab_pos);
        std::string device_status = last_device.substr(tab_pos + 1);
        if (device_status.find("device") != std::string::npos) {
            devices.push_back(device_serial);
        }
    }

    return devices;
}


// http://aospxref.com/android-13.0.0_r3/xref/packages/modules/adb/OVERVIEW.TXT 
// http://aospxref.com/android-13.0.0_r3/xref/packages/modules/adb/SERVICES.TXT
AdbClient::AdbResponse AdbClient::executeCommand(const std::string& command) {
    return sendCommand(command);
}


const std::string AdbClient::ADB_HOST = "127.0.0.1";
