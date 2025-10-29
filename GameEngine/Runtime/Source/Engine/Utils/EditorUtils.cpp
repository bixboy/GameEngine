#include "Engine/Utils/EditorUtils.h"
#include "Core/Logger.h"

#include <cstdlib>
#include <string>
#include <filesystem>

#if defined(_WIN32)
    #include <windows.h>
#endif

namespace BixEngine::Utils
{
    namespace
    {
        String g_EditorCommand;

        bool IsExecutableAvailable(const std::string& exeName)
        {
            char buffer[MAX_PATH];
            DWORD result = SearchPathA(nullptr, exeName.c_str(), ".exe", MAX_PATH, buffer, nullptr);
            return result != 0;
        }

        void DetectDefaultEditor()
        {
            if (IsExecutableAvailable("rider64"))
                g_EditorCommand = "rider64";
            
            else if (IsExecutableAvailable("code"))
                g_EditorCommand = "code";
            
            else if (IsExecutableAvailable("devenv"))
                g_EditorCommand = "devenv";
            
            else
            {
                g_EditorCommand = "notepad";
                LOG_WARNING("[EditorUtils] Aucun éditeur connu trouvé dans le PATH. Fallback sur Notepad.");
            }

            LOG_INFO("[EditorUtils] Default code editor set to: " + g_EditorCommand);
        }
    }

    void SetPreferredCodeEditor(const std::string& command)
    {
        g_EditorCommand = command;
    }

    const std::string& GetPreferredCodeEditor()
    {
        if (g_EditorCommand.empty())
            DetectDefaultEditor();
        
        return g_EditorCommand;
    }

    void OpenFileInCodeEditor(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            LOG_WARNING(std::string("[EditorUtils] File not found: ") + path.string());
            return;
        }

        if (g_EditorCommand.empty())
            DetectDefaultEditor();

        std::string fullPath = path.string();
        std::string cmd = "cmd.exe /C \"" + g_EditorCommand + " \"" + fullPath + "\"\"";

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi{};
        BOOL success = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

        if (!success)
        {
            DWORD err = GetLastError();
            LOG_ERROR("[EditorUtils] Failed to open file in editor via CreateProcessA. Error code: " + std::to_string(err));

            std::wstring wpath = path.wstring();
            std::wstring wcmd = std::wstring(g_EditorCommand.begin(), g_EditorCommand.end());
            ShellExecuteW(nullptr, L"open", wcmd.c_str(), wpath.c_str(), nullptr, SW_SHOWNORMAL);
        }
        else
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }


    std::uint64_t HashFNV1a(std::string_view str)
    {
        constexpr std::uint64_t kOffset = 1469598103934665603ull;
        constexpr std::uint64_t kPrime = 1099511628211ull;

        std::uint64_t hash = kOffset;
        for (unsigned char c : str)
            hash = (hash ^ c) * kPrime;
        return hash;
    }
}
