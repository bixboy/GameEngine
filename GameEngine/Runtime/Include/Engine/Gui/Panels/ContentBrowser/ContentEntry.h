#pragma once
#include "Core/Containers/String.h"
#include <filesystem>


namespace BixEngine::Gui
{
    // ─────────────────────────────────────────────
    // 📁  Types d'entrées du Content Browser
    // ─────────────────────────────────────────────
    
    enum class ContentType
    {
        Directory = 0,
        File,
        Script,
        Actor,
    };

    // ─────────────────────────────────────────────
    // 🧱  Métadonnées d'une entrée de contenu
    // ─────────────────────────────────────────────
    
    struct ContentEntry
    {
        String name{};                           // Nom de l’entrée
        std::filesystem::path path{};            // Chemin complet
        ContentType type{ContentType::File};     // Type de contenu
        String extension{};                      // Extension du fichier

        std::filesystem::path headerPath{};      // Chemin du header .h
        std::filesystem::path sourcePath{};      // Chemin du source .cpp

        // ─────────────────────────────────────────────
        // 🧩  Helpers de type et de statut
        // ─────────────────────────────────────────────
        
        [[nodiscard]] bool IsDirectory() const noexcept { return type == ContentType::Directory; }
        [[nodiscard]] bool IsFile() const noexcept { return type == ContentType::File; }
        [[nodiscard]] bool IsScript() const noexcept { return type == ContentType::Script; }
        [[nodiscard]] bool IsActor() const noexcept { return type == ContentType::Actor; }

        [[nodiscard]] bool HasHeader() const noexcept { return !headerPath.empty(); }
        [[nodiscard]] bool HasSource() const noexcept { return !sourcePath.empty(); }

        [[nodiscard]] bool IsAsset() const noexcept { return IsActor() || IsScript(); }

        [[nodiscard]] String SelectionKey() const;
    };

    // ─────────────────────────────────────────────
    // 🧭  Fonctions utilitaires
    // ─────────────────────────────────────────────
    
    const char* GetIcon(ContentType type);
    int GetSortPriority(ContentType type);
}
