#include "pch.h"
#include "CommandExecution.h"
#include <memory>
#include <thread>
#include <tchar.h>

CommandExecution::CommandResult CommandExecution::ExecCommand(const CString& command, bool asAdmin)
{
    return ExecCommand(std::vector<CString>{command}, asAdmin);
}

CommandExecution::CommandResult CommandExecution::ExecCommand(const std::vector<CString>& commands, bool asAdmin)
{
    if (commands.empty()) {
        return CommandResult();
    }

    CString commandLine = _T("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"");
    for (const auto& command : commands) {
        commandLine += EscapePowerShellCommand(command) + _T("; ");
    }
    commandLine += _T("\"");

    return ExecuteCommands(commandLine, asAdmin);
}

std::future<CommandExecution::CommandResult> CommandExecution::ExecCommandAsync(const CString& command, bool asAdmin)
{
    return ExecCommandAsync(std::vector<CString>{command}, asAdmin);
}

std::future<CommandExecution::CommandResult> CommandExecution::ExecCommandAsync(const std::vector<CString>& commands, bool asAdmin)
{
    if (commands.empty()) {
        std::promise<CommandResult> resultPromise;
        resultPromise.set_value(CommandResult());
        return resultPromise.get_future();
    }

    CString commandLine = _T("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"");
    for (const auto& command : commands) {
        commandLine += EscapePowerShellCommand(command) + _T("; ");
    }
    commandLine += _T("\"");

    std::promise<CommandResult> resultPromise;
    std::future<CommandResult> resultFuture = resultPromise.get_future();

    std::thread([commandLine, asAdmin, resultPromise = std::move(resultPromise)]() mutable {
        AsyncExecutor(commandLine, asAdmin, std::move(resultPromise));
        }).detach();

    return resultFuture;
}

CommandExecution::CommandResult CommandExecution::ExecuteCommands(const CString& commandLine, bool asAdmin)
{
    CommandResult commandResult;

    SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &securityAttributes, 0)) {
        commandResult.errorMsg = _T("Failed to create pipe");
        return commandResult;
    }

    STARTUPINFO startupInfo = { sizeof(STARTUPINFO) };
    startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startupInfo.wShowWindow = SW_HIDE;
    startupInfo.hStdOutput = hWritePipe;
    startupInfo.hStdError = hWritePipe;

    PROCESS_INFORMATION processInfo = { 0 };

    TCHAR* commandLineCopy = _tcsdup(commandLine);

    DWORD creationFlags = asAdmin ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW;
    BOOL success = CreateProcess(
        nullptr,
        commandLineCopy,
        nullptr,
        nullptr,
        TRUE,
        creationFlags,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
    );

    if (!success) {
        commandResult.errorMsg = _T("Failed to create process");
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return commandResult;
    }

    CloseHandle(hWritePipe);

    const int bufferSize = 4096;
    std::unique_ptr<CHAR[]> buffer(new CHAR[bufferSize]);
    DWORD bytesRead;
    CString output;

    while (ReadFile(hReadPipe, buffer.get(), bufferSize - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += CString(buffer.get());
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    GetExitCodeProcess(processInfo.hProcess, &commandResult.result);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    CloseHandle(hReadPipe);

    if (commandResult.result == 0) commandResult.successMsg = output;
    else commandResult.errorMsg = output;

    ATLTRACE(_T("Result: %d | Success: %s | Error: %s\n"),
        commandResult.result,
        commandResult.successMsg,
        commandResult.errorMsg);

    return commandResult;
}

void CommandExecution::AsyncExecutor(const CString& commandLine, bool asAdmin, std::promise<CommandResult>&& resultPromise)
{
    CommandResult result = ExecuteCommands(commandLine, asAdmin);
    resultPromise.set_value(std::move(result));
}

CString CommandExecution::EscapePowerShellCommand(const CString& command)
{
    CString escaped = command;
    escaped.Replace(_T("\""), _T("\\\"")); // 转义双引号
    //escaped.Replace(_T("$"), _T("`$")); // 转义美元符号 这里的操作如果是直接在终端中执行确实是需要的，但是通过“运行”的方式，也就是直接创建进程的方式，是不需要的
    //return _T("\"") + escaped + _T("\"");
    return escaped;
}
