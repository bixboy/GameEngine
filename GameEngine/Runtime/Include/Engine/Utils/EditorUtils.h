#pragma once
#include <filesystem>

namespace BixEngine::EditorUtils
{
    /**
     * @brief Ouvre un fichier source dans l’éditeur de code configuré.
     * 
     * @param path Chemin absolu du fichier à ouvrir.
     */
    void OpenFileInCodeEditor(const std::filesystem::path& path);

    /**
     * @brief Définit la commande d’exécution utilisée pour ouvrir les fichiers.
     * Exemple : "code", "rider64", "devenv", etc.
     */
    void SetPreferredCodeEditor(const std::string& command);

    /**
     * @brief Retourne la commande actuellement utilisée.
     */
    const std::string& GetPreferredCodeEditor();
    
    /**
     * @brief Calcule un hash 64-bit FNV-1a à partir d'une chaîne de caractères.
     * Utilisé pour générer des identifiants uniques (ex: Actor Editors, Panels).
     */
    [[nodiscard]] std::uint64_t HashFNV1a(std::string_view str);
}
