#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
#include "Containers/String.h"

namespace BixEngine::EditorUtils
{
    class Utilities
    {
    public:

        /** Définit la commande utilisée pour ouvrir les fichiers (ex: "code", "rider64", "devenv"). */
        static void SetPreferredCodeEditor(const std::string& command);

        /** Retourne la commande actuellement utilisée, détectée automatiquement si nécessaire. */
        static const std::string& GetPreferredCodeEditor();
        

        /** Ouvre un fichier dans l’éditeur configuré (Rider, VSCode, etc.). */
        static void OpenFileInCodeEditor(const std::filesystem::path& path);

        /** Ouvre un dossier ou sélectionne un fichier dans l’explorateur système. */
        static void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);
        

        /** Calcule un hash FNV-1a 64-bit à partir d’une chaîne. */
        [[nodiscard]] static std::uint64_t HashFNV1a(std::string_view str);

    private:
        static std::string s_EditorCommand;
        static void DetectDefaultEditor();
        static bool IsExecutableAvailable(const std::string& exeName);
    };
}
