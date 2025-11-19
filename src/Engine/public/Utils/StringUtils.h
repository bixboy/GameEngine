#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include "Containers/String.h"

namespace BixEngine::StringUtils
{
    class Utilities
    {
    public:

        /** Retourne une copie d'une chaîne convertie en minuscules (ASCII-safe). */
        static std::string ToLowerCopy(std::string value);

        /** Extrait une valeur texte d’un contenu JSON brut sans parsing complet. */
        static std::string ExtractJsonString(const std::string& source, std::string_view key);

        /** Compare deux chaînes en ignorant la casse. Retourne true si `query` est vide ou présent dans `value`. */
        static bool MatchesSearch(const String& value, const String& query);
        

        /** Ouvre un dossier ou sélectionne un fichier dans l’explorateur système. */
        static void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory);

        // Transforme un path en identifiant string
        static String MakeSafeIdentifier(const std::string& raw);

        // Supprime les caractères '\r'
        static void TrimCarriageReturn(std::string& v);
    };
}
