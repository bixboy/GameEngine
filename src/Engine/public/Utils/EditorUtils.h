#pragma once
#include <filesystem>
#include <string>
#include <cstdint>
#include "Containers/String.h"


namespace BixEngine::EditorUtils
{
    // ─────────────────────────────────────────────
    // Configuration de l’éditeur

    /** Définit la commande utilisée pour ouvrir les fichiers (ex: "code", "rider64", "devenv"). */
    void SetPreferredCodeEditor(const std::string& command);

    /** Retourne la commande actuellement utilisée, détectée automatiquement si nécessaire. */
    const std::string& GetPreferredCodeEditor();

    // ─────────────────────────────────────────────
    // Ouverture de fichiers / dossiers

    /** Ouvre un fichier dans l’éditeur configuré (Rider, VSCode, etc.). */
    void OpenFileInCodeEditor(const std::filesystem::path& path);

    /** Ouvre un dossier ou sélectionne un fichier dans l’explorateur système. */
    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);

    /** Calcule un hash FNV-1a 64-bit à partir d’une chaîne. */
    [[nodiscard]] std::uint64_t HashFNV1a(std::string_view str);
}
