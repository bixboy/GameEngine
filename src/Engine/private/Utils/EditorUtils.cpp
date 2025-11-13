#include "Utils/EditorUtils.h"
#include "Logger.h"

#include <filesystem>
#include <string>
#include <windows.h>

namespace BixEngine::EditorUtils
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
            {
                g_EditorCommand = "rider64";
            }
            else if (IsExecutableAvailable("code"))
            {
                g_EditorCommand = "code";
            }
            else if (IsExecutableAvailable("devenv"))
            {
                g_EditorCommand = "devenv";
            }
            else
            {
                g_EditorCommand = "notepad";
                LOG_WARNING("[EditorUtils] Aucun éditeur connu trouvé dans le PATH. Utilisation de Notepad.");
            }

            LOG_INFO("[EditorUtils] Éditeur de code par défaut : " + g_EditorCommand);
        }
    }

    // ─────────────────────────────────────────────
    // Configuration
    // ─────────────────────────────────────────────

    void SetPreferredCodeEditor(const std::string& command)
    {
        g_EditorCommand = command;
        LOG_INFO("[EditorUtils] Éditeur préféré défini sur : " + command);
    }

    const std::string& GetPreferredCodeEditor()
    {
        if (g_EditorCommand.empty())
            DetectDefaultEditor();

        return g_EditorCommand;
    }

    // ─────────────────────────────────────────────
    // Ouverture d’un fichier dans l’éditeur
    // ─────────────────────────────────────────────

    void OpenFileInCodeEditor(const std::filesystem::path& path)
    {
        namespace fs = std::filesystem;

        if (!fs::exists(path))
        {
            LOG_WARNING("[EditorUtils] Fichier introuvable : " + path.string());
            return;
        }

        if (g_EditorCommand.empty())
            DetectDefaultEditor();

        std::string fullPath = path.string();
        std::string cmd = "cmd.exe /C \"" + g_EditorCommand + " \"" + fullPath + "\"\"";

        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi{};
        BOOL success = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr,
                                      FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

        if (!success)
        {
            const DWORD err = GetLastError();
            LOG_ERROR(
                "[EditorUtils] Échec de CreateProcessA pour " + g_EditorCommand +
                ". Code erreur : " + std::to_string(err));

            // Fallback
            std::wstring wpath = path.wstring();
            std::wstring wcmd(g_EditorCommand.begin(), g_EditorCommand.end());
            ShellExecuteW(nullptr, L"open", wcmd.c_str(), wpath.c_str(), nullptr, SW_SHOWNORMAL);
            return;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // ─────────────────────────────────────────────
    // Ouvrir l’explorateur Windows
    // ─────────────────────────────────────────────

    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
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

    std::uint64_t HashFNV1a(std::string_view str)
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
