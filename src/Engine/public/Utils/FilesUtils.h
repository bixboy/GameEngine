#pragma once
#include <filesystem>
#include <string>
#include "Containers/String.h"


namespace BixEngine::FileUtils
{
    /**
     * @brief Tente de créer un répertoire (et ses parents) de manière sécurisée.
     * @param dir  Le chemin à créer.
     * @param outError Chaîne de sortie contenant l’erreur si échec.
     * @return True si succès, False si erreur.
     */
    bool TryCreateDir(const std::filesystem::path& dir, String& outError);

    /**
     * @brief Supprime un fichier ou répertoire de manière sécurisée.
     * @param path Chemin du fichier à supprimer.
     * @param recursive Si vrai, supprime récursivement.
     * @param outError Chaîne d’erreur en cas d’échec.
     * @return True si suppression réussie, False sinon.
     */
    bool TryRemove(const std::filesystem::path& path, bool recursive, String& outError);

    /**
     * @brief Copie un fichier de manière sécurisée, avec création du dossier cible si nécessaire.
     * @param source Chemin du fichier source.
     * @param destination Chemin du fichier destination.
     * @param overwrite Si vrai, remplace un fichier existant.
     * @param outError Chaîne d’erreur remplie en cas d’échec.
     * @return True si la copie réussit, False sinon.
     */
    bool TryCopyFile(const std::filesystem::path& source, const std::filesystem::path& destination, bool overwrite, String& outError);

    /**
     * @brief Renomme ou déplace un fichier ou dossier de manière sécurisée.
     *
     * @param source       Chemin du fichier ou dossier source à renommer/déplacer.
     * @param destination  Nouveau chemin de destination (nom ou emplacement).
     * @param overwrite    Si vrai, supprime le fichier de destination s’il existe déjà.
     * @param outError     Chaîne d’erreur remplie en cas d’échec.
     * @return True si le renommage ou déplacement a réussi, False sinon.
     */    
    bool TryRename(const std::filesystem::path& source, const std::filesystem::path& destination, bool overwrite, String& outError);

    /**
     * @brief Loggue une erreur et la stocke dans une String.
     * @param outError La String d’erreur à remplir.
     * @param message  Message d’erreur à enregistrer.
     * @param alsoLog  Si vrai, envoie aussi le message au Logger.
     */
    bool LogAndStoreError(String& outError, const String& message, bool alsoLog = true);

    /**
     * @brief Comparaison de chaînes insensible à la casse.
     */
    bool CaseInsensitiveLess(const std::string& a, const std::string& b);

    /**
     * @brief Écrit du texte dans un fichier de manière sécurisée..
     *
     * @param path       Chemin du fichier à écrire.
     * @param content    Contenu texte à écrire.
     * @param outError   Chaîne d’erreur remplie en cas d’échec.
     * @return True si l’écriture réussit, False sinon.
     */
    bool TryWriteFile(const std::filesystem::path& path, const std::string& content, String& outError);


    String ExtractDisplayName(const std::filesystem::path& path);


    std::filesystem::path NormalizePath(const std::filesystem::path& path);

    std::filesystem::path ResolveUserConfigPath(const char* fileName);

}
