#pragma once
#include <filesystem>
#include <vector>
#include "Containers/String.h"
#include "Ressources/Atlas/SpriteAtlasUtils.h"


namespace BixEngine::resources
{
    struct SpriteAtlasCreationParams
    {
        std::filesystem::path texturePath{};
        float columns{1.f};
        float rows{1.f};
        int padding{0};
        int margin{0};
    };

    class SpriteAtlasFactory
    {
    public:
        [[nodiscard]] static bool ValidateCreationParams(const SpriteAtlasCreationParams& params, String& outError);
        [[nodiscard]] static bool CreateAtlasFile(const std::filesystem::path& atlasPath, const SpriteAtlasCreationParams& params, String& outError);
        [[nodiscard]] static bool SaveAtlasFile(const std::filesystem::path& atlasPath, const SpriteAtlasDefinition& definition, const std::vector<SpriteAnimationDefinition>& animations, String& outError);
    };
}
