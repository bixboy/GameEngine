#pragma once
#include <filesystem>
#include <vector>
#include "Containers/String.h"
#include "Render/SpriteFrame.h"


namespace BixEngine::resources
{
    class Texture;

    /**
     * @brief Describes the static layout of an atlas.
     */
    struct SpriteAtlasDefinition
    {
        int columns{0};
        int rows{0};
        int padding{0};
        int margin{0};
        String texturePath{};
    };

    /**
     * @brief Describes a single animation entry from a sprite atlas document.
     */
    struct SpriteAnimationDefinition
    {
        String name{};
        float frameRate{0.0f};
        bool loop{true};
        std::vector<size_t> frames{};
    };

    namespace SpriteAtlasUtils
    {
        /**
         * @brief Tente de détecter automatiquement la grille d'un atlas à partir d’une texture.
         *
         * @param texturePath Chemin vers une texture PNG existante.
         * @param outCols Colonne détectées.
         * @param outRows Lignes détectées.
         */
        bool AutoDetectGrid(const std::filesystem::path& texturePath, int& outCols, int& outRows);
        
        /**
         * @brief Charge et analyse un fichier JSON .atlas.
         *
         * @param path Fichier .atlas à lire.
         * @param outDefinition Sortie : informations sur la grille.
         * @param outAnimations Sortie : liste des animations chargées.
         */
        bool ParseAtlasFile(const String& path, SpriteAtlasDefinition& outDefinition, std::vector<SpriteAnimationDefinition>& outAnimations);

        /**
         * @brief Génère la liste complète des frames d’un atlas en fonction d'une grille.
         *
         * @param texture Texture source (déjà chargée).
         * @param columns Nombre de colonnes.
         * @param rows Nombre de lignes.
         * @param padding Padding entre les frames.
         * @param margin Marge autour de tous les frames.
         */
        std::vector<SpriteFrame> GenerateFrames(Texture& texture, int columns, int rows, int padding, int margin);
    }
}
