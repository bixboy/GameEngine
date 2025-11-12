#include "Ressources/SpriteAtlasUtils.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Logger.h"
#include "Ressources/Texture.h"


namespace BixEngine::resources
{
    namespace
    {
        [[nodiscard]] bool ValidateGrid(int columns, int rows)
        {
            return columns > 0 && rows > 0;
        }
    }

    bool SpriteAtlasUtils::ParseAtlasFile(const String& path, SpriteAtlasDefinition& outDefinition,
                                          std::vector<SpriteAnimationDefinition>& outAnimations)
    {
        std::ifstream stream(path.c_str());
        if (!stream.is_open())
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: unable to open file " + path);
            return false;
        }

        nlohmann::json document;
        try
        {
            stream >> document;
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: invalid JSON in " + path + ": " + ex.what());
            return false;
        }

        if (!document.contains("texture"))
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: missing 'texture' field in " + path);
            return false;
        }

        outDefinition.texturePath = document["texture"].get<std::string>().c_str();
        outDefinition.columns = document.value("columns", 1);
        outDefinition.rows = document.value("rows", 1);
        outDefinition.padding = document.value("padding", 0);
        outDefinition.margin = document.value("margin", 0);

        if (!ValidateGrid(outDefinition.columns, outDefinition.rows))
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: invalid grid dimensions in " + path);
            return false;
        }

        outAnimations.clear();
        if (document.contains("animations"))
        {
            try
            {
                for (const auto& entry : document["animations"])
                {
                    SpriteAnimationDefinition anim;
                    anim.name = entry.value("name", "").c_str();
                    anim.frameRate = entry.value("frameRate", 0.0f);
                    anim.loop = entry.value("loop", true);

                    if (entry.contains("frames"))
                    {
                        for (const auto& index : entry["frames"])
                        {
                            anim.frames.push_back(index.get<size_t>());
                        }
                    }

                    outAnimations.push_back(std::move(anim));
                }
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: invalid animation block in " + path + ": " + ex.what());
                outAnimations.clear();
            }
        }

        return true;
    }

    std::vector<SpriteFrame> SpriteAtlasUtils::GenerateFrames(Texture& texture, int columns, int rows, int padding,
                                                              int margin)
    {
        std::vector<SpriteFrame> frames;
        if (!ValidateGrid(columns, rows))
            return frames;

        const float texWidth = static_cast<float>(texture.GetWidth());
        const float texHeight = static_cast<float>(texture.GetHeight());
        const float cellWidth = (texWidth - 2.0f * static_cast<float>(margin) - static_cast<float>((columns - 1) *
            padding)) / static_cast<float>(columns);
        const float cellHeight = (texHeight - 2.0f * static_cast<float>(margin) - static_cast<float>((rows - 1) *
            padding)) / static_cast<float>(rows);

        if (cellWidth <= 0.0f || cellHeight <= 0.0f)
        {
            LOG_ERROR("SpriteAtlasUtils::GenerateFrames: invalid cell size computed for atlas.");
            return frames;
        }

        frames.reserve(static_cast<size_t>(columns * rows));

        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                Math::Rect rect{
                    static_cast<float>(margin) + static_cast<float>(column) * (cellWidth + static_cast<float>(padding)),
                    static_cast<float>(margin) + static_cast<float>(row) * (cellHeight + static_cast<float>(padding)),
                    cellWidth,
                    cellHeight
                };

                SpriteFrame frame;
                frame.texture = &texture;
                frame.uvRect = rect;
                frames.push_back(frame);
            }
        }

        return frames;
    }
}
