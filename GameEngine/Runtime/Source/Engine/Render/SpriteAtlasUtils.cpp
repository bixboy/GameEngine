#include "Engine/Render/SpriteAtlasUtils.h"
#include "Engine/Render/SpriteFramePool.h"
#include "Engine/Render/Texture.h"
#include <fstream>
#include <sstream>

namespace BixEngine::Render
{
    namespace
    {
        [[nodiscard]] std::string LoadFile(const std::string& path)
        {
            std::ifstream file(path);
            if (!file.is_open())
                return {};
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        [[nodiscard]] float ExtractFloat(const std::string& source, const std::string& key, float defaultValue)
        {
            const std::string token = "\"" + key + "\"";
            size_t pos = source.find(token);
            if (pos == std::string::npos)
                return defaultValue;
            pos = source.find(':', pos);
            if (pos == std::string::npos)
                return defaultValue;
            size_t end = source.find_first_of(",}", pos + 1);
            const std::string value = source.substr(pos + 1, end - pos - 1);
            try
            {
                return std::stof(value);
            }
            catch (...)
            {
                return defaultValue;
            }
        }

        [[nodiscard]] int ExtractInt(const std::string& source, const std::string& key, int defaultValue)
        {
            return static_cast<int>(ExtractFloat(source, key, static_cast<float>(defaultValue)));
        }

        [[nodiscard]] bool ExtractBool(const std::string& source, const std::string& key, bool defaultValue)
        {
            const std::string token = "\"" + key + "\"";
            size_t pos = source.find(token);
            if (pos == std::string::npos)
                return defaultValue;
            pos = source.find(':', pos);
            if (pos == std::string::npos)
                return defaultValue;
            const size_t truePos = source.find("true", pos);
            const size_t falsePos = source.find("false", pos);
            if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos))
                return true;
            if (falsePos != std::string::npos)
                return false;
            return defaultValue;
        }

        [[nodiscard]] std::vector<int> ParseIndices(const std::string& block)
        {
            std::vector<int> indices;
            std::stringstream stream(block);
            char ch;
            int value = 0;
            while (stream >> ch)
            {
                if ((ch >= '0' && ch <= '9') || ch == '-')
                {
                    stream.putback(ch);
                    if (stream >> value)
                        indices.push_back(value);
                }
            }
            return indices;
        }

        [[nodiscard]] Math::Rect ExtractRect(const std::string& block)
        {
            const float x = ExtractFloat(block, "x", 0.0f);
            const float y = ExtractFloat(block, "y", 0.0f);
            const float w = ExtractFloat(block, "w", 0.0f);
            const float h = ExtractFloat(block, "h", 0.0f);
            return Math::Rect(x, y, w, h);
        }
    }

    std::vector<SpriteFrame> SpriteAtlasUtils::LoadFramesFromAtlas(Texture& texture, int columns, int rows, int padding, int margin)
    {
        std::vector<SpriteFrame> frames;
        if (columns <= 0 || rows <= 0)
            return frames;

        const float texWidth = static_cast<float>(texture.GetWidth());
        const float texHeight = static_cast<float>(texture.GetHeight());
        const float cellWidth = (texWidth - 2.0f * margin - static_cast<float>((columns - 1) * padding)) / static_cast<float>(columns);
        const float cellHeight = (texHeight - 2.0f * margin - static_cast<float>((rows - 1) * padding)) / static_cast<float>(rows);

        SpriteFramePool& pool = SpriteFramePool::Get();

        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                Math::Rect rect{margin + column * (cellWidth + padding),
                                margin + row * (cellHeight + padding),
                                cellWidth,
                                cellHeight};
                frames.emplace_back(pool.Acquire(&texture, rect));
            }
        }

        return frames;
    }

    std::vector<SpriteFrame> SpriteAtlasUtils::LoadFramesFromJSON(Texture& texture, const std::string& jsonPath)
    {
        std::vector<SpriteFrame> frames;
        const std::string contents = LoadFile(jsonPath);
        if (contents.empty())
            return frames;

        SpriteFramePool& pool = SpriteFramePool::Get();
        size_t pos = 0;
        while ((pos = contents.find("\"frame\"", pos)) != std::string::npos)
        {
            size_t start = contents.find('{', pos);
            size_t end = contents.find('}', start);
            if (start == std::string::npos || end == std::string::npos)
                break;
            Math::Rect rect = ExtractRect(contents.substr(start, end - start));
            frames.emplace_back(pool.Acquire(&texture, rect));
            pos = end;
        }

        if (frames.empty())
        {
            pos = 0;
            while ((pos = contents.find("\"x\"", pos)) != std::string::npos)
            {
                size_t end = contents.find('}', pos);
                Math::Rect rect = ExtractRect(contents.substr(pos, end - pos));
                if (rect.Width > 0.0f && rect.Height > 0.0f)
                    frames.emplace_back(pool.Acquire(&texture, rect));
                pos = end != std::string::npos ? end : contents.size();
            }
        }

        return frames;
    }

    std::vector<SpriteAnimation> SpriteAtlasUtils::LoadAnimationsFromAsset(const std::string& assetPath, Texture& texture)
    {
        std::vector<SpriteAnimation> animations;
        const std::string contents = LoadFile(assetPath);
        if (contents.empty())
            return animations;

        const int columns = ExtractInt(contents, "columns", 1);
        const int rows = ExtractInt(contents, "rows", 1);
        const int padding = ExtractInt(contents, "padding", 0);
        const int margin = ExtractInt(contents, "margin", 0);

        std::vector<SpriteFrame> frames = LoadFramesFromAtlas(texture, columns, rows, padding, margin);
        if (frames.empty())
            return animations;

        size_t pos = 0;
        while ((pos = contents.find("\"name\"", pos)) != std::string::npos)
        {
            size_t colon = contents.find(':', pos);
            size_t quoteStart = contents.find('"', colon + 1);
            size_t quoteEnd = contents.find('"', quoteStart + 1);
            if (colon == std::string::npos || quoteStart == std::string::npos || quoteEnd == std::string::npos)
                break;

            std::string name = contents.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            size_t blockStart = contents.rfind('{', pos);
            size_t blockEnd = contents.find('}', quoteEnd);
            if (blockStart == std::string::npos || blockEnd == std::string::npos)
            {
                pos = quoteEnd;
                continue;
            }

            const std::string block = contents.substr(blockStart, blockEnd - blockStart + 1);
            SpriteAnimation animation{};
            animation.Name = name.c_str();
            animation.FrameRate = ExtractFloat(block, "frameRate", 12.0f);
            animation.bLoop = ExtractBool(block, "loop", true);
            animation.bPingPong = ExtractBool(block, "pingPong", false);

            size_t framesToken = block.find("\"frames\"");
            if (framesToken != std::string::npos)
            {
                size_t listStart = block.find('[', framesToken);
                size_t listEnd = block.find(']', listStart);
                if (listStart != std::string::npos && listEnd != std::string::npos)
                {
                    const std::vector<int> indices = ParseIndices(block.substr(listStart + 1, listEnd - listStart - 1));
                    for (int index : indices)
                    {
                        if (index >= 0 && static_cast<size_t>(index) < frames.size())
                        {
                            animation.Frames.push_back(frames[static_cast<size_t>(index)]);
                        }
                    }
                }
            }

            size_t eventsToken = block.find("\"events\"");
            if (eventsToken != std::string::npos)
            {
                size_t listStart = block.find('[', eventsToken);
                size_t listEnd = block.find(']', listStart);
                if (listStart != std::string::npos && listEnd != std::string::npos)
                {
                    std::string eventsBlock = block.substr(listStart + 1, listEnd - listStart - 1);
                    size_t eventPos = 0;
                    while ((eventPos = eventsBlock.find("\"frame\"", eventPos)) != std::string::npos)
                    {
                        size_t eventEnd = eventsBlock.find('}', eventPos);
                        std::string eventChunk = eventsBlock.substr(eventPos, eventEnd - eventPos);
                        SpriteEvent evt{};
                        evt.FrameIndex = static_cast<size_t>(ExtractInt(eventChunk, "frame", 0));
                        size_t nameToken = eventsBlock.rfind("\"name\"", eventPos);
                        if (nameToken != std::string::npos)
                        {
                            size_t nameColon = eventsBlock.find(':', nameToken);
                            size_t nameStart = eventsBlock.find('"', nameColon + 1);
                            size_t nameEnd = eventsBlock.find('"', nameStart + 1);
                            if (nameStart != std::string::npos && nameEnd != std::string::npos)
                            {
                                evt.Name = eventsBlock.substr(nameStart + 1, nameEnd - nameStart - 1).c_str();
                            }
                        }
                        animation.Events.push_back(std::move(evt));
                        eventPos = eventEnd;
                    }
                }
            }

            if (!animation.Frames.empty())
            {
                animations.push_back(std::move(animation));
            }

            pos = blockEnd;
        }

        return animations;
    }
}
