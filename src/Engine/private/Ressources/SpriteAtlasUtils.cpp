#include "Ressources/SpriteAtlasUtils.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cctype>
#include <optional>
#include "Logger.h"
#include "Ressources/ResourceManager.h"
#include "Ressources/Texture.h"


namespace BixEngine::resources
{
    namespace fs = std::filesystem;

    // ──────────────────────────────────────────────
    // INTERNAL HELPERS
    // ──────────────────────────────────────────────

    namespace
    {
        [[nodiscard]] bool ValidateGrid(int columns, int rows)
        {
            return columns > 0 && rows > 0;
        }

        int ExtractFrameCount(const std::string& name)
        {
            if (name.empty()) return 0;

            size_t i = name.size();
            while (i > 0 && std::isdigit(static_cast<unsigned char>(name[i - 1])))
                --i;

            if (i == name.size() || i == 0)
                return 0;

            char sep = name[i - 1];
            if (sep != '_' && sep != '-' && sep != ' ')
                return 0;

            try { return std::stoi(name.substr(i)); }
            catch (...) { return 0; }
        }

        struct GridInfo { int cols, rows, pad, margin; };

        std::optional<GridInfo> DetectGridExact(int frames, int w, int h)
        {
            if (frames <= 0) return std::nullopt;

            float targetAspect = (float)w / (float)h;
            struct { int c, r; float score; } best{0, 0, 99999.f};

            for (int r = 1; r <= frames; ++r)
            {
                if (frames % r)
                    continue;
                
                int c = frames / r;

                if (w % c || h % r)
                    continue;

                float aspect = (float)c / (float)r;
                float score  = std::abs(aspect - targetAspect);

                if (score < best.score)
                    best = {c, r, score};
            }

            if (best.c == 0)
                return std::nullopt;

            return GridInfo{best.c, best.r, 0, 0};
        }

        std::optional<GridInfo> DetectGridAspect(int w, int h)
        {
            if (w >= h && w % h == 0)
                return GridInfo{w / h, 1, 0, 0};

            if (h > w && h % w == 0)
                return GridInfo{1, h / w, 0, 0};

            return GridInfo{1, 1, 0, 0};
        }
    }

    // ──────────────────────────────────────────────
    // PUBLIC — Auto-detection
    // ──────────────────────────────────────────────

    bool SpriteAtlasUtils::AutoDetectGrid(const fs::path& texturePath, int& outCols, int& outRows)
    {
        outCols = outRows = 1;

        if (!fs::exists(texturePath))
            return false;

        auto texture = ResourceManager::Get().Get<Texture>(texturePath.generic_string().c_str());
        if (!texture) return false;

        const int w = texture->GetWidth();
        const int h = texture->GetHeight();

        if (w <= 0 || h <= 0)
            return false;

        std::optional<GridInfo> detected;

        const int frameCount = ExtractFrameCount(texturePath.stem().string());
        if (frameCount > 0)
            detected = DetectGridExact(frameCount, w, h);

        if (!detected.has_value())
            detected = DetectGridAspect(w, h);

        if (!detected.has_value())
            return false;

        outCols = std::max(detected->cols, 1);
        outRows = std::max(detected->rows, 1);

        return true;
    }

    // ──────────────────────────────────────────────
    // PARSE .atlas FILE
    // ──────────────────────────────────────────────

    bool SpriteAtlasUtils::ParseAtlasFile(const String& path, SpriteAtlasDefinition& outDefinition, std::vector<SpriteAnimationDefinition>& outAnimations)
    {
        std::ifstream stream(path.c_str());
        if (!stream.is_open())
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: unable to open file " + path);
            return false;
        }

        nlohmann::json doc;
        try { stream >> doc; }
        catch (const std::exception& ex)
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: invalid JSON: " + String(ex.what()));
            return false;
        }

        if (!doc.contains("texture"))
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: missing 'texture'");
            return false;
        }

        outDefinition.texturePath = doc["texture"].get<std::string>().c_str();
        outDefinition.columns = doc.value("columns", 1);
        outDefinition.rows    = doc.value("rows", 1);
        outDefinition.padding = doc.value("padding", 0);
        outDefinition.margin  = doc.value("margin", 0);

        if (!ValidateGrid(outDefinition.columns, outDefinition.rows))
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: invalid grid");
            return false;
        }

        // animations
        outAnimations.clear();
        if (doc.contains("animations"))
        {
            try
            {
                for (const auto& animJson : doc["animations"])
                {
                    SpriteAnimationDefinition anim;
                    anim.name = animJson.value("name", "").c_str();
                    anim.frameRate = animJson.value("frameRate", 0.0f);
                    anim.loop = animJson.value("loop", true);

                    if (animJson.contains("frames"))
                    {
                        for (const auto& f : animJson["frames"])
                            anim.frames.push_back(f.get<size_t>());
                    }

                    outAnimations.push_back(std::move(anim));
                }
            }
            catch (...)
            {
                LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: bad animation block");
                outAnimations.clear();
            }
        }

        return true;
    }

    // ──────────────────────────────────────────────
    // FRAME GENERATION
    // ──────────────────────────────────────────────

    std::vector<SpriteFrame>
    SpriteAtlasUtils::GenerateFrames(Texture& texture, int columns, int rows, int padding, int margin)
    {
        std::vector<SpriteFrame> frames;

        columns = std::max(columns, 1);
        rows = std::max(rows, 1);
        padding = std::max(padding, 0);
        margin = std::max(margin, 0);

        if (!ValidateGrid(columns, rows))
            return frames;

        const float texW = (float)texture.GetWidth();
        const float texH = (float)texture.GetHeight();

        const float cellW = (texW - 2.f * margin - (columns - 1) * padding) / columns;
        const float cellH = (texH - 2.f * margin - (rows - 1) * padding) / rows;

        if (cellW <= 0 || cellH <= 0)
        {
            LOG_ERROR("SpriteAtlasUtils::GenerateFrames: invalid cell size");
            return frames;
        }

        frames.reserve((size_t)(columns * rows));

        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < columns; ++c)
            {
                Math::Rect rect{
                    margin + c * (cellW + padding),
                    margin + r * (cellH + padding),
                    cellW,
                    cellH
                };

                SpriteFrame f;
                f.texture = &texture;
                f.uvRect  = rect;

                frames.push_back(f);
            }
        }

        return frames;
    }
}
