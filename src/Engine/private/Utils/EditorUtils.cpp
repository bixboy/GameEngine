#include "Utils/EditorUtils.h"
#include "Logger.h"
#include <windows.h>

namespace BixEngine::EditorUtils
{
    std::string Utilities::s_EditorCommand;

    
    // ─────────────────────────────────────────────
    // Utilitaires internes
    // ─────────────────────────────────────────────
    bool Utilities::IsExecutableAvailable(const std::string& exeName)
    {
        char buffer[MAX_PATH];
        DWORD result = SearchPathA(nullptr, exeName.c_str(), ".exe", MAX_PATH, buffer, nullptr);
        return result != 0;
    }

    void Utilities::DetectDefaultEditor()
    {
        if (IsExecutableAvailable("rider64"))
            s_EditorCommand = "rider64";
        
        else if (IsExecutableAvailable("code"))
            s_EditorCommand = "code";
        
        else if (IsExecutableAvailable("devenv"))
            s_EditorCommand = "devenv";
        
        else
        {
            s_EditorCommand = "notepad";
            LOG_WARNING("[EditorUtils] Aucun éditeur connu trouvé dans le PATH. Utilisation de Notepad.");
        }

        LOG_INFO("[EditorUtils] Éditeur de code par défaut : " + s_EditorCommand);
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

        std::string fullPath = path.string();
        std::string cmd = "cmd.exe /C \"" + s_EditorCommand + " \"" + fullPath + "\"\"";

        STARTUPINFOA si{sizeof(si)};
        PROCESS_INFORMATION pi{};
        BOOL success = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                                      FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

        if (!success)
        {
            const DWORD err = GetLastError();
            LOG_ERROR("[EditorUtils] Échec de CreateProcessA pour " + s_EditorCommand +
                      ". Code erreur : " + std::to_string(err));

            // Fallback
            std::wstring wpath = path.wstring();
            std::wstring wcmd(s_EditorCommand.begin(), s_EditorCommand.end());
            ShellExecuteW(nullptr, L"open", wcmd.c_str(), wpath.c_str(), nullptr, SW_SHOWNORMAL);
            return;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
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
