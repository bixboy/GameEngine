#include "Utils/EditorUtils.h"
#include "Logger.h"
#include <string>
#include <filesystem>
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


    // ─────────────────────────────────────────────────────────────
    // Configuration de l’éditeur
    // ─────────────────────────────────────────────────────────────

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

    // ─────────────────────────────────────────────────────────────
    // Ouverture de fichiers dans l’éditeur
    // ─────────────────────────────────────────────────────────────

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
                "[EditorUtils] Échec de CreateProcessA pour " + g_EditorCommand + ". Code erreur : " + std::to_string(
                    err));

            std::wstring wpath = path.wstring();
            auto wcmd = std::wstring(g_EditorCommand.begin(), g_EditorCommand.end());
            ShellExecuteW(nullptr, L"open", wcmd.c_str(), wpath.c_str(), nullptr, SW_SHOWNORMAL);

            return;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
