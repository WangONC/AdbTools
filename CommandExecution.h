#pragma once
#include <atlstr.h>
#include <Windows.h>
#include <vector>
#include <future>
using namespace ATL;


class CommandExecution
{
public:
    struct CommandResult {
        DWORD result = -1;
        CString errorMsg;
        CString successMsg;
    };

    // 同步方法
    static CommandResult ExecCommand(const CString& command, bool asAdmin = false);
    static CommandResult ExecCommand(const std::vector<CString>& commands, bool asAdmin = false);

    // 异步方法
    static std::future<CommandResult> ExecCommandAsync(const CString& command, bool asAdmin = false);
    static std::future<CommandResult> ExecCommandAsync(const std::vector<CString>& commands, bool asAdmin = false);

private:
    static CommandResult ExecuteCommands(const CString& commandLine, bool asAdmin);
    static void AsyncExecutor(const CString& commandLine, bool asAdmin, std::promise<CommandResult>&& resultPromise);
    static CString EscapePowerShellCommand(const CString& command);
};

