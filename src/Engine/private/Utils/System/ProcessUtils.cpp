#include "Utils/System/ProcessUtils.h"
#include "Debug/Logger.h"
#include <windows.h>

namespace BixEngine::System
{
    std::wstring ToWString(const std::string& str)
    {
        if (str.empty())
            return L"";
        
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    ProcessResult ProcessUtils::RunCommand(const std::filesystem::path& executable, const std::vector<String>& args)
    {
        ProcessResult result;
        
        std::wstring cmdLine = L"\"" + executable.wstring() + L"\"";
        for (const auto& arg : args)
        {
            cmdLine += L" " + ToWString(arg.Std());
        }

        // Buffers
        std::vector cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back(0);

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            result.exitCode = static_cast<int>(exitCode);
            result.success = (result.exitCode == 0);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            result.success = false;
            LOG_ERROR("ProcessUtils: Failed to launch " + String(executable.string().c_str()));
        }
        
        return result;
    }

    bool ProcessUtils::LaunchDetached(const std::filesystem::path& executable, const std::string& args)
    {
        std::wstring wExe = executable.wstring();
        std::wstring wArgs = ToWString(args);
        std::wstring fullCmd = L"\"" + wExe + L"\" " + wArgs;

        std::vector cmdBuffer(fullCmd.begin(), fullCmd.end());
        cmdBuffer.push_back(0);

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        }
        
        return false;
    }
}