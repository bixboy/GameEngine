#include "Utils/Editor/EditorUtils.h"
#include "Debug/Logger.h"
#include <windows.h>

namespace BixEngine::EditorUtils
{
    std::string Utilities::s_EditorCommand;

    
    // ─────────────────────────────────────────────
    // Utilitaires internes
    // ─────────────────────────────────────────────
    namespace
    {
        void LaunchDetachedProcess(const std::wstring& cmd, const std::wstring& args)
        {
            // We concatenate cmd + " " + args for lpCommandLine
            // CreateProcess requires a mutable string for command line
            std::wstring fullCmd = cmd + L" " + args;
            std::vector<wchar_t> cmdBuffer(fullCmd.begin(), fullCmd.end());
            cmdBuffer.push_back(0); // Null terminator

            STARTUPINFOW si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            // DETACHED_PROCESS (0x00000008) -> No console window inherited
            // CREATE_BREAKAWAY_FROM_JOB (0x01000000) -> Detach from parent job (if any)
            DWORD flags = DETACHED_PROCESS | 0x01000000; 

            if (CreateProcessW(
                NULL,               // No module name (use command line)
                cmdBuffer.data(),   // Command line
                NULL,               // Process handle not inheritable
                NULL,               // Thread handle not inheritable
                FALSE,              // Set handle inheritance to FALSE
                flags,              // Creation flags
                NULL,               // Use parent's environment block
                NULL,               // Use parent's starting directory
                &si,                // Pointer to STARTUPINFO structure
                &pi)                // Pointer to PROCESS_INFORMATION structure
            )
            {
                // We don't need to wait or hold handles
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                LOG_INFO("[EditorUtils] Detached process launched successfully.");
            }
            else
            {
                LOG_ERROR("[EditorUtils] Failed to launch detached process. Error: " + std::to_string(GetLastError()));
            }
        }
    }

    bool Utilities::IsExecutableAvailable(const std::string& exeName)
    {
        char buffer[MAX_PATH];
        DWORD result = SearchPathA(nullptr, exeName.c_str(), ".exe", MAX_PATH, buffer, nullptr);
        return result != 0;
    }

    void Utilities::DetectDefaultEditor()
    {
        // Detect in order of preference for "heavy" development: Rider -> Visual Studio -> VS Code
        if (IsExecutableAvailable("rider64"))
            s_EditorCommand = "rider64";
        else if (IsExecutableAvailable("devenv"))
            s_EditorCommand = "devenv";
        else if (IsExecutableAvailable("code"))
            s_EditorCommand = "code";
        else 
        {
            s_EditorCommand = ""; // Empty implies "Use System Association"
        }

        LOG_INFO("[EditorUtils] Code editor strategy: " + (s_EditorCommand.empty() ? "System Association" : s_EditorCommand));
    }

    // ─────────────────────────────────────────────
    // Configuration
    // ─────────────────────────────────────────────
    void Utilities::SetPreferredCodeEditor(const std::string& command)
    {
        s_EditorCommand = command;
        LOG_INFO("[EditorUtils] Éditeur préféré défini sur : " + command);
    }

    const std::string& Utilities::GetPreferredCodeEditor()
    {
        if (s_EditorCommand.empty())
            DetectDefaultEditor();

        return s_EditorCommand;
    }

    // ─────────────────────────────────────────────
    // Ouverture de fichiers / dossiers
    // ─────────────────────────────────────────────
    void Utilities::OpenFileInCodeEditor(const std::filesystem::path& path)
    {
        namespace fs = std::filesystem;

        if (!fs::exists(path))
        {
            LOG_WARNING("[EditorUtils] Fichier introuvable : " + path.string());
            return;
        }

        if (s_EditorCommand.empty())
            DetectDefaultEditor();

        // 1. If explicit editor is set, try to use it with workspace/solution context
        if (!s_EditorCommand.empty())
        {
             std::wstring wpath = path.wstring();
             std::wstring editorCmd(s_EditorCommand.begin(), s_EditorCommand.end());

             // Determine project root (look for xmake.lua)
             fs::path projectRoot = path.parent_path();
             fs::path current = projectRoot;
             bool foundRoot = false;
             while (!current.empty() && current != current.root_path())
             {
                 if (fs::exists(current / "xmake.lua"))
                 {
                     projectRoot = current;
                     foundRoot = true;
                     break;
                 }
                 current = current.parent_path();
             }

             // Find Solution File (.sln) if we have a project root
             fs::path solutionPath;
             if (foundRoot)
             {
                 // Check root
                 for (auto const& dir_entry : fs::directory_iterator(projectRoot))
                 {
                     if (dir_entry.path().extension() == ".sln")
                     {
                         solutionPath = dir_entry.path();
                         break;
                     }
                 }
                 // Check vsxmake2022/ folder 
                 if (solutionPath.empty() && fs::exists(projectRoot / "vsxmake2022"))
                 {
                     for (auto const& dir_entry : fs::directory_iterator(projectRoot / "vsxmake2022"))
                     {
                         if (dir_entry.path().extension() == ".sln")
                         {
                             solutionPath = dir_entry.path();
                             break;
                         }
                     }
                 }
             }

             std::wstring wSolution = solutionPath.wstring();
             std::wstring wRoot = projectRoot.wstring();
             std::wstring args;
             bool handled = false;

             if (s_EditorCommand == "rider64" && !solutionPath.empty())
             {
                 // rider64 "Solution.sln" --line <line> "File"
                 args = L"\"" + wSolution + L"\" --line 1 \"" + wpath + L"\"";
                 LaunchDetachedProcess(editorCmd, args);
                 handled = true;
             }
             else if (s_EditorCommand == "devenv" && !solutionPath.empty())
             {
                 // devenv "Solution.sln" /Edit "File"
                 args = L"\"" + wSolution + L"\" /Edit \"" + wpath + L"\"";
                 LaunchDetachedProcess(editorCmd, args);
                 handled = true;
             }
             else if (s_EditorCommand == "code") 
             {
                 // code "Root" -g "File"
                 if (foundRoot)
                     args = L"\"" + wRoot + L"\" -g \"" + wpath + L"\"";
                 else
                     args = L"-g \"" + wpath + L"\"";
                     
                 LaunchDetachedProcess(editorCmd, args);
                 handled = true;
             }
             
             if (handled)
                 return;

             LOG_WARNING("[EditorUtils] Fallback strategy for editor '" + s_EditorCommand + "'.");
        }

        // 2. Fallback: Open file with system default association
        ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    void Utilities::ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
    {
        if (path.empty())
            return;

        std::wstring wpath = path.wstring();

        if (isDirectory)
        {
            // Ouvre un dossier
            ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        else
        {
            // Sélectionne le fichier dans l'explorateur
            std::wstring cmd = L"/select,\"" + wpath + L"\"";
            ShellExecuteW(nullptr, L"open", L"explorer.exe", cmd.c_str(), nullptr, SW_SHOWNORMAL);
        }
    }

    // ─────────────────────────────────────────────
    // Hash FNV-1a 64-bit
    // ─────────────────────────────────────────────
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
