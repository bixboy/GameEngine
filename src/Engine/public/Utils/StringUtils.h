#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include "Containers/String.h"


namespace BixEngine::StringUtils
{
    /**
     * @brief Retourne une copie d'une chaîne convertie en minuscules (ASCII-safe).
     */
    [[nodiscard]] std::string ToLowerCopy(std::string value);

    /**
     * @brief Extrait une valeur texte d’un contenu JSON brut sans parsing complet.
     */
    [[nodiscard]] std::string ExtractJsonString(const std::string& source, std::string_view key);

    /**
     * @brief Compare deux chaînes en ignorant la casse.
     * Retourne true si `query` est vide ou présent dans `value`.
     */
    [[nodiscard]] bool MatchesSearch(const String& value, const String& query);

    /**
     * @brief Ouvre un dossier ou sélectionne un fichier dans l’explorateur système.
     * @param path Chemin absolu à ouvrir.
     * @param isDirectory  True pour ouvrir le dossier, false pour sélectionner un fichier.
     */
    void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);
}
