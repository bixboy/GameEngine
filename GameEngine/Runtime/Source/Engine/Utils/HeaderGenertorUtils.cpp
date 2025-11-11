#include "Engine/Utils/HeaderGenertorUtils.h"
#include "Core/Logger.h"
#include "Core/FileUtils.h"
#include <filesystem>
#include <string>

#if defined(_WIN32)
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif

namespace BixEngine::HeaderGeneratorUtils
{
    void RunBixHeaderTool(const std::filesystem::path& toolPath,
                          const std::filesystem::path& headerPath)
    {
        std::wstring tool = L"\"" + toolPath.wstring() + L"\"";
        std::wstring header = L"\"" + headerPath.wstring() + L"\"";
        std::wstring cmdLine = tool + L" --single " + header;

        LOG_INFO("Launching BixHeaderTool: " + String(std::string(cmdLine.begin(), cmdLine.end()).c_str()));

        STARTUPINFOW si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);

        if (!CreateProcessW(nullptr, cmdLine.data(),
                            nullptr, nullptr, FALSE, 0,
                            nullptr, nullptr, &si, &pi))
        {
            LOG_ERROR("Failed to launch BixHeaderTool.exe");
            return;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0)
            LOG_WARNING("BixHeaderTool exited with code " + String::FromInt(exitCode));
        else
            LOG_INFO("BixHeaderTool finished successfully.");
    }
}
