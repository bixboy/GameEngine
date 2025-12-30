#include "Ressources/Atlas/SpriteAtlasFactory.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Debug/Logger.h"


namespace BixEngine::Resources
{
    namespace
    {
        void AppendError(String& storage, const String& message)
        {
            storage = message.data();
            LOG_ERROR(String("[SpriteAtlasFactory] " + String(message.data())));
        }

        [[nodiscard]] bool EnsureParentDirectory(const std::filesystem::path& targetPath, String& outError)
        {
            if (targetPath.empty())
            {
                AppendError(outError, "Target path is empty.");
                return false;
            }

            const auto directory = targetPath.parent_path();
            if (directory.empty())
                return true;

            if (std::filesystem::exists(directory))
                return true;

            std::error_code error;
            std::filesystem::create_directories(directory, error);
            if (error)
            {
                AppendError(outError, "Failed to create directory: " + String(directory.generic_string().c_str()));
                return false;
            }

            return std::filesystem::exists(directory);
        }

        void SerializeDefinition(nlohmann::json& document, const SpriteAtlasDefinition& definition)
        {
            document["texture"] = definition.texturePath.c_str(); 
            document["columns"] = definition.columns;
            document["rows"] = definition.rows;
            document["padding"] = definition.padding;
            document["margin"] = definition.margin;
        }

        void SerializeAnimations(nlohmann::json& document, const std::vector<SpriteAnimationDefinition>& animations)
        {
            nlohmann::json animationArray = nlohmann::json::array();
            for (const auto& animation : animations)
            {
                nlohmann::json animJson;
                animJson["name"] = animation.name.c_str();
                animJson["frameRate"] = animation.frameRate;
                animJson["loop"] = animation.loop;

                nlohmann::json framesJson = nlohmann::json::array();
                for (size_t frameIndex : animation.frames)
                    framesJson.push_back(frameIndex);

                animJson["frames"] = std::move(framesJson);
                animationArray.push_back(std::move(animJson));
            }

            document["animations"] = std::move(animationArray);
        }
    }

    bool SpriteAtlasFactory::ValidateCreationParams(const SpriteAtlasCreationParams& params, String& outError)
    {
        if (params.texturePath.empty())
        {
            AppendError(outError, "Texture path is required.");
            return false;
        }

        if (!std::filesystem::exists(params.texturePath))
        {
            AppendError(outError, "Texture not found: " + String(params.texturePath.generic_string().c_str()));
            return false;
        }

        if (params.columns < 1 || params.rows < 1)
        {
            AppendError(outError, "Columns and rows must be >= 1.");
            return false;
        }

        if (params.padding < 0 || params.margin < 0)
        {
            AppendError(outError, "Padding and margin cannot be negative.");
            return false;
        }

        return true;
    }

    bool SpriteAtlasFactory::CreateAtlasFile(const std::filesystem::path& atlasPath, const SpriteAtlasCreationParams& params, String& outError)
    {
        if (!ValidateCreationParams(params, outError))
            return false;

        if (atlasPath.empty())
        {
            AppendError(outError, "Invalid atlas path.");
            return false;
        }

        if (!EnsureParentDirectory(atlasPath, outError))
            return false;

        if (std::filesystem::exists(atlasPath))
        {
            AppendError(outError, "Atlas already exists: " + String(atlasPath.generic_string().c_str()));
            return false;
        }

        SpriteAtlasDefinition definition{};
        definition.columns = static_cast<int>(params.columns);
        definition.rows = static_cast<int>(params.rows);
        definition.padding = params.padding;
        definition.margin = params.margin;
        
        definition.texturePath = String(params.texturePath.filename().generic_string().c_str());

        nlohmann::json document;
        SerializeDefinition(document, definition);
        document["animations"] = nlohmann::json::array();

        std::ofstream stream(atlasPath, std::ios::trunc);
        if (!stream.is_open())
        {
            AppendError(outError, "Unable to create atlas file: " + String(atlasPath.generic_string().c_str()));
            return false;
        }

        stream << document.dump(4);
        return true;
    }

    bool SpriteAtlasFactory::SaveAtlasFile(const std::filesystem::path& atlasPath, const SpriteAtlasDefinition& definition, const std::vector<SpriteAnimationDefinition>& animations, String& outError)
    {
        if (atlasPath.empty())
        {
            AppendError(outError, "Invalid atlas path.");
            return false;
        }

        if (!EnsureParentDirectory(atlasPath, outError))
            return false;

        if (definition.columns < 1 || definition.rows < 1)
        {
            AppendError(outError, "Atlas grid dimensions must be >= 1.");
            return false;
        }

        if (definition.padding < 0 || definition.margin < 0)
        {
            AppendError(outError, "Padding and margin cannot be negative.");
            return false;
        }

        nlohmann::json document;
        SerializeDefinition(document, definition);
        SerializeAnimations(document, animations);

        std::ofstream stream(atlasPath, std::ios::trunc);
        if (!stream.is_open())
        {
            AppendError(outError, "Unable to open atlas for writing: " + String(atlasPath.generic_string().c_str()));
            return false;
        }

        stream << document.dump(4);
        return true;
    }
}