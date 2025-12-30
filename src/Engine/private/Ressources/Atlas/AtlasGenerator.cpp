#include "Ressources/Atlas/AtlasGenerator.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "Debug/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

namespace BixEngine::Resources
{
    namespace
    {
        [[nodiscard]] bool IsPng(const std::filesystem::path& path)
        {
            const auto ext = path.extension().string();
            return ext == ".png" || ext == ".PNG";
        }

        [[nodiscard]] bool EnsureFolder(const std::filesystem::path& folder)
        {
            if (std::filesystem::exists(folder))
                return true;
            return std::filesystem::create_directories(folder);
        }

        [[nodiscard]] std::string BuildTextureFileName(const std::filesystem::path& folder)
        {
            const std::string animationName = folder.filename().string();
            const std::string parentName = folder.has_parent_path() ? folder.parent_path().filename().string() : "";

            if (!parentName.empty())
                return parentName + "_" + animationName + ".png";
            
            return animationName + ".png";
        }

        [[nodiscard]] std::string BuildAtlasFileName(const std::filesystem::path& folder)
        {
            const std::string parentName = folder.has_parent_path() ? folder.parent_path().filename().string() : "";
            if (!parentName.empty())
                return parentName + ".atlas";
            
            return folder.filename().string() + ".atlas";
        }

        void WriteAtlasJson(const std::filesystem::path& jsonPath, const std::string& textureFile, int columns, int rows, int padding, int margin,
            float frameRate, bool loop, size_t frameCount, const std::string& animationName)
        {
            nlohmann::json document;
            document["texture"] = textureFile;
            document["columns"] = columns;
            document["rows"] = rows;
            document["padding"] = padding;
            document["margin"] = margin;

            nlohmann::json animation;
            animation["name"] = animationName;
            animation["frameRate"] = frameRate;
            animation["loop"] = loop;
            animation["frames"] = nlohmann::json::array();
            
            for (size_t i = 0; i < frameCount; ++i)
                animation["frames"].push_back(i);

            document["animations"] = nlohmann::json::array({animation});

            std::ofstream output(jsonPath);
            if (output.is_open())
            {
                output << document.dump(4);
            }
            else
            {
                LOG_ERROR("AtlasGenerator: Impossible d'écrire le JSON " + jsonPath.string());
            }
        }
    }

    // ------ Atlas Generator ------
    
    bool AtlasGenerator::GenerateAtlas(const std::filesystem::path& frameDirectory, int columns, int rows, int padding, int margin, float frameRate, bool loop)
    {
        // 1. Validation du dossier
        if (!std::filesystem::exists(frameDirectory) || !std::filesystem::is_directory(frameDirectory))
        {
            LOG_ERROR("AtlasGenerator: Dossier invalide " + frameDirectory.string());
            return false;
        }

        // 2. Récupération des fichiers PNG
        std::vector<std::filesystem::path> frameFiles;
        for (const auto& entry : std::filesystem::directory_iterator(frameDirectory))
        {
            if (entry.is_regular_file() && IsPng(entry.path()))
                frameFiles.push_back(entry.path());
        }
        
        std::sort(frameFiles.begin(), frameFiles.end());

        if (frameFiles.empty())
        {
            LOG_ERROR("AtlasGenerator: Aucun PNG trouvé dans " + frameDirectory.string());
            return false;
        }

        // 3. Calcul des dimensions de la grille (Columns x Rows)
        columns = std::max(columns, 0);
        rows = std::max(rows, 0);
        padding = std::max(padding, 0);
        margin = std::max(margin, 0);

        if (columns == 0 && rows == 0)
        {
            float sqrtCount = std::sqrt(static_cast<float>(frameFiles.size()));
            
            columns = static_cast<int>(std::ceil(sqrtCount));
            rows = static_cast<int>((frameFiles.size() + columns - 1) / columns);
        }
        else if (columns == 0)
        {
            columns = static_cast<int>((frameFiles.size() + rows - 1) / rows);
        }
        else if (rows == 0)
        {
            rows = static_cast<int>((frameFiles.size() + columns - 1) / columns);
        }

        // 4. Chargement des images en RAM
        int frameWidth = 0;
        int frameHeight = 0;
        std::vector<std::vector<uint8_t>> loadedFrames;
        loadedFrames.reserve(frameFiles.size());

        for (const auto& framePath : frameFiles)
        {
            int width = 0, height = 0, channels = 0;
            stbi_uc* pixels = stbi_load(framePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
            
            if (!pixels)
            {
                LOG_ERROR("AtlasGenerator: Échec chargement " + framePath.string());
                return false;
            }

            if (frameWidth == 0 && frameHeight == 0)
            {
                frameWidth = width;
                frameHeight = height;
            }
            else if (width != frameWidth || height != frameHeight)
            {
                LOG_ERROR("AtlasGenerator: Dimensions incohérentes dans " + framePath.string() + ". Attendu: " + std::to_string(frameWidth) +
                    "x" + std::to_string(frameHeight));
                
                stbi_image_free(pixels);
                return false;
            }

            std::vector<uint8_t> buffer(static_cast<size_t>(width * height * 4));
            std::copy(pixels, pixels + buffer.size(), buffer.begin());
            
            stbi_image_free(pixels);
            loadedFrames.push_back(std::move(buffer));
        }

        // 5. Création du buffer de l'Atlas
        const int atlasWidth = columns * frameWidth + (columns - 1) * padding + 2 * margin;
        const int atlasHeight = rows * frameHeight + (rows - 1) * padding + 2 * margin;

        std::vector<uint8_t> atlasPixels(static_cast<size_t>(atlasWidth * atlasHeight * 4), 0);

        // 6. Blit (Copie) des frames dans l'atlas
        for (size_t index = 0; index < loadedFrames.size(); ++index)
        {
            const int column = static_cast<int>(index % columns);
            const int rowIndex = static_cast<int>(index / columns);
            
            const int destX = margin + column * (frameWidth + padding);
            const int destY = margin + rowIndex * (frameHeight + padding);

            const uint8_t* source = loadedFrames[index].data();
            
            for (int y = 0; y < frameHeight; ++y)
            {
                uint8_t* destination = atlasPixels.data() + static_cast<size_t>((destY + y) * atlasWidth + destX) * 4;
                const uint8_t* srcRow = source + static_cast<size_t>(y * frameWidth) * 4;
                
                std::copy(srcRow, srcRow + static_cast<size_t>(frameWidth) * 4, destination);
            }
        }

        // 7. Écriture des fichiers de sortie
        const std::filesystem::path outputDirectory = frameDirectory.parent_path();
        if (!outputDirectory.empty() && !EnsureFolder(outputDirectory))
        {
            LOG_ERROR("AtlasGenerator: Impossible de créer le dossier de sortie.");
            return false;
        }

        const std::string textureFileName = BuildTextureFileName(frameDirectory);
        const std::filesystem::path texturePath = outputDirectory / textureFileName;

        if (stbi_write_png(texturePath.string().c_str(), atlasWidth, atlasHeight, 4, atlasPixels.data(), atlasWidth * 4) == 0)
        {
            LOG_ERROR("AtlasGenerator: Échec écriture PNG " + texturePath.string());
            return false;
        }

        const std::string atlasFileName = BuildAtlasFileName(frameDirectory);
        const std::filesystem::path atlasPath = outputDirectory / atlasFileName;
        
        WriteAtlasJson(atlasPath, textureFileName, columns, rows, padding, margin, frameRate, loop,
            loadedFrames.size(),
            frameDirectory.filename().string());

        LOG_INFO("AtlasGenerator: Génération réussie -> " + atlasPath.string());
        return true;
    }
}