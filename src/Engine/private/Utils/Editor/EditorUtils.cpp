#include "Utils/Editor/EditorUtils.h"
#include "Debug/Logger.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace BixEngine::EditorUtils
{
    std::string Utilities::s_EditorCommand;

    namespace
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

        void LaunchDetachedProcess(const std::wstring& cmd, const std::wstring& args)
        {
            std::wstring fullCmd = L"\"" + cmd + L"\" " + args;
            
            std::vector cmdBuffer(fullCmd.begin(), fullCmd.end());
            cmdBuffer.push_back(0);

            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            DWORD flags = DETACHED_PROCESS | CREATE_NEW_CONSOLE; 

            if (CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else
            {
                LOG_ERROR("[EditorUtils] Failed to launch process. Error: " + std::to_string(GetLastError()));
            }
        }
        
        std::filesystem::path FindSolutionFile(const std::filesystem::path& root)
        {
             namespace fs = std::filesystem;
             if (!fs::exists(root))
                 return {};

             for (const auto& entry : fs::directory_iterator(root))
             {
                 if (entry.path().extension() == ".sln")
                     return entry.path();
             }

             fs::path vsxmake = root / "vsxmake2022";
             if (fs::exists(vsxmake))
             {
                 for (const auto& entry : fs::directory_iterator(vsxmake))
                 {
                     if (entry.path().extension() == ".sln")
                         return entry.path();
                 }   
             }

             return {};
        }
    }

    bool Utilities::IsExecutableAvailable(const std::string& exeName)
    {
#ifdef _WIN32
        char buffer[MAX_PATH];
        return SearchPathA(nullptr, exeName.c_str(), ".exe", MAX_PATH, buffer, nullptr) != 0;
#else
        return false;
#endif
    }

    void Utilities::DetectDefaultEditor()
    {
        if (IsExecutableAvailable("rider64"))
        {
            s_EditorCommand = "rider64";
        }
        else if (IsExecutableAvailable("code"))
        {
            s_EditorCommand = "code";
        }
        else if (IsExecutableAvailable("devenv"))
        {
            s_EditorCommand = "devenv";
        }
        else
        {
            s_EditorCommand = "";
        }

        LOG_INFO("[EditorUtils] Detected editor: " + (s_EditorCommand.empty() ? "None (System Default)" : s_EditorCommand));
    }

    void Utilities::SetPreferredCodeEditor(const std::string& command)
    {
        s_EditorCommand = command;
    }

    const std::string& Utilities::GetPreferredCodeEditor()
    {
        if (s_EditorCommand.empty())
            DetectDefaultEditor();
        
        return s_EditorCommand;
    }

    void Utilities::OpenFileInCodeEditor(const std::filesystem::path& path, int line)
    {
        namespace fs = std::filesystem;
        if (!fs::exists(path))
            return;

        if (s_EditorCommand.empty())
            DetectDefaultEditor();

        if (!s_EditorCommand.empty())
        {
             fs::path projectRoot = path.parent_path();
             fs::path current = projectRoot;
             bool foundRoot = false;

             while (!current.empty() && current != current.root_path())
             {
                 if (fs::exists(current / "xmake.lua")) {
                     projectRoot = current;
                     foundRoot = true;
                     break;
                 }
                 current = current.parent_path();
             }

             fs::path solutionPath = foundRoot ? FindSolutionFile(projectRoot) : fs::path();
             
             std::wstring wpath = path.wstring();
             std::wstring wSolution = solutionPath.wstring();
             std::wstring editorCmd = ToWString(s_EditorCommand);
             std::wstring args;
            
             if (s_EditorCommand == "rider64")
             {
                 // Rider: "Solution.sln"
                 if (!solutionPath.empty())
                     args = L"\"" + wSolution + L"\"";
                 
                 if (line > 0)
                     args += L" --line " + std::to_wstring(line);
                 
                 args += L" \"" + wpath + L"\"";
                 
                 LaunchDetachedProcess(editorCmd, args);
                 return;
             }
            
             if (s_EditorCommand == "code") 
             {
                 // VS Code: -g "File.cpp"
                 if (foundRoot)
                     args = L"\"" + projectRoot.wstring() + L"\"";
                 
                 args += L" -g \"" + wpath + L"\"";
                 if (line > 0)
                     args += L":" + std::to_wstring(line);
                     
                 LaunchDetachedProcess(editorCmd, args);
                 return;
             }
            
             if (s_EditorCommand == "devenv")
             {
                 if (!solutionPath.empty())
                    args = L"\"" + wSolution + L"\"";
                 
                 args += L" /Edit \"" + wpath + L"\"";
                 
                 LaunchDetachedProcess(editorCmd, args);
                 return;
             }
             
             LOG_WARNING("[EditorUtils] Unknown editor '" + s_EditorCommand + "', falling back to system default.");
        }

        ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    void Utilities::ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
    {
        if (path.empty())
            return;
        
        std::wstring wpath = path.wstring();

        if (isDirectory)
        {
            ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        else
        {
            std::wstring cmd = L"/select,\"" + wpath + L"\"";
            ShellExecuteW(nullptr, L"open", L"explorer.exe", cmd.c_str(), nullptr, SW_SHOWNORMAL);
        }
    }

    std::uint64_t Utilities::HashFNV1a(std::string_view str)
    {
        constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
        constexpr std::uint64_t FNV_PRIME  = 1099511628211ull;
        std::uint64_t hash = FNV_OFFSET;
        
        for (unsigned char c : str)
        {
            hash ^= c;
            hash *= FNV_PRIME;
        }
        
        return hash;
    }
}