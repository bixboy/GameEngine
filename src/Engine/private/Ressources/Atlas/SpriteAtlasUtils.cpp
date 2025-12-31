#include "Ressources/Atlas/SpriteAtlasUtils.h"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cctype>
#include <optional>
#include <cmath>
#include "Debug/Logger.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/RessourcesClass/Texture.h"
#include "SDL_image.h"
#include "SDL3/SDL_surface.h"


namespace BixEngine::Resources
{
    namespace fs = std::filesystem;

    namespace
    {
        [[nodiscard]] bool ValidateGrid(int columns, int rows)
        {
            return columns > 0 && rows > 0;
        }

        int ExtractFrameCount(const std::string& name)
        {
            if (name.empty())
                return 0;

            size_t i = name.size();
            while (i > 0 && std::isdigit(static_cast<unsigned char>(name[i - 1])))
                --i;

            if (i == name.size() || i == 0)
                return 0;

            char sep = name[i - 1];
            if (sep != '_' && sep != '-' && sep != ' ')
                return 0;

            try
            {
                return std::stoi(name.substr(i));
            }
            catch (...)
            {
                return 0;
            }
        }

        struct GridInfo { int cols, rows; };

        std::optional<GridInfo> DetectGridExact(int frames, int w, int h)
        {
            if (frames <= 0)
                return std::nullopt;

            float targetAspect = static_cast<float>(w) / static_cast<float>(h);
            struct
            {
                int c, r;
                float score;
            }
            best{0, 0, 99999.f};

            for (int r = 1; r <= frames; ++r)
            {
                if (frames % r)
                    continue;
                
                int c = frames / r;
                if (w % c || h % r)
                    continue;

                float aspect = static_cast<float>(c) / static_cast<float>(r);
                float score  = std::abs(aspect - targetAspect);

                if (score < best.score)
                    best = {c, r, score};
            }

            if (best.c == 0)
                return std::nullopt;
            
            return GridInfo{best.c, best.r};
        }

        std::optional<GridInfo> DetectGridFromPixels(const fs::path& path)
        {
            SDL_Surface* rawSurf = IMG_Load(path.string().c_str());
            if (!rawSurf)
                return std::nullopt;

            SDL_Surface* surf = SDL_ConvertSurface(rawSurf, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(rawSurf);
            if (!surf)
                return std::nullopt;

            if (SDL_LockSurface(surf) != 0)
            {
                SDL_DestroySurface(surf);
                return std::nullopt;
            }

            int w = surf->w;
            int h = surf->h;
            Uint32* pixels = static_cast<Uint32*>(surf->pixels);

            auto HasAlpha = [&](int x, int y)
            {
                Uint8 r, g, b, a;
                const SDL_PixelFormatDetails* details = SDL_GetPixelFormatDetails(surf->format);
                SDL_GetRGBA(pixels[y * w + x], details, nullptr, &r, &g, &b, &a);
                
                return a > 10;
            };

            std::vector<int> islandWidths;
            bool inBlock = false;
            int startX = 0;
            for (int x = 0; x < w; ++x)
            {
                bool hasContent = false;
                for (int y = 0; y < h; ++y)
                {
                    if (HasAlpha(x, y))
                    {
                        hasContent = true; break;
                    }
                }
                
                if (hasContent)
                {
                    if (!inBlock)
                    {
                        inBlock = true; startX = x;
                    }
                }
                else
                {
                    if (inBlock)
                    {
                        inBlock = false;
                        if (x - startX > 2) islandWidths.push_back(x - startX);
                    }
                }
            }
            
            if (inBlock)
                islandWidths.push_back(w - startX);

            std::vector<int> islandHeights;
            inBlock = false;
            int startY = 0;
            
            for (int y = 0; y < h; ++y)
            {
                bool hasContent = false;
                for (int x = 0; x < w; ++x)
                {
                    if (HasAlpha(x, y))
                    {
                        hasContent = true; break;
                    }
                }
                
                if (hasContent)
                {
                    if (!inBlock)
                    {
                        inBlock = true; startY = y;
                    }
                }
                else
                {
                    if (inBlock)
                    {
                        inBlock = false;
                        if (y - startY > 2)
                            islandHeights.push_back(y - startY);
                    }
                }
            }
            
            if (inBlock)
                islandHeights.push_back(h - startY);

            SDL_UnlockSurface(surf);
            SDL_DestroySurface(surf);

            int cols = 1;
            if (!islandWidths.empty())
            {
                std::ranges::sort(islandWidths);
                int medianW = islandWidths[islandWidths.size() / 2];
                
                if (medianW > 1)
                {
                    cols = std::max(1, static_cast<int>(std::round(static_cast<float>(w) / static_cast<float>(medianW))));
                }
                else
                {
                    cols = static_cast<int>(islandWidths.size());
                }
            }

            int rows = 1;
            if (!islandHeights.empty())
            {
                std::ranges::sort(islandHeights);
                int medianH = islandHeights[islandHeights.size() / 2];
                
                if (medianH > 1)
                {
                    rows = std::max(1, static_cast<int>(std::round(static_cast<float>(h) / static_cast<float>(medianH))));
                }
                else
                {
                    rows = static_cast<int>(islandHeights.size());
                }
            }

            LOG_INFO("DetectGridFromPixels: Found " + std::to_string(cols) + " cols, " + std::to_string(rows) + " rows (Median)");
            return GridInfo{cols, rows};
        }

        std::optional<GridInfo> DetectGridAspect(int w, int h)
        {
            if (w >= h && w % h == 0)
                return GridInfo{w / h, 1};
            
            if (h > w && h % w == 0)
                return GridInfo{1, h / w};
            
            return GridInfo{1, 1};
        }
    }

    bool SpriteAtlasUtils::AutoDetectGrid(const fs::path& texturePath, int& outCols, int& outRows)
    {
        outCols = outRows = 1;
        if (!fs::exists(texturePath))
            return false;

        int w = 0, h = 0;

        if (auto tex = ResourceManager::Get().Get<Texture>(String(texturePath.generic_string().c_str())))
        {
            w = static_cast<int>(tex->GetWidth());
            h = static_cast<int>(tex->GetHeight());
        }

        std::optional<GridInfo> detected;
        int frameCount = ExtractFrameCount(texturePath.stem().string());
        
        if (frameCount > 0 && w > 0 && h > 0)
            detected = DetectGridExact(frameCount, w, h);
        
        if (!detected)
            detected = DetectGridFromPixels(texturePath);
        
        if (!detected || (detected->cols == 1 && detected->rows == 1)) 
        {
            if (w > 0 && h > 0)
                detected = DetectGridAspect(w, h);
        }

        if (detected)
        {
            outCols = detected->cols;
            outRows = detected->rows;
            return true;
        }
        
        return false;
    }

    bool SpriteAtlasUtils::ParseAtlasFile(const String& path, SpriteAtlasDefinition& outDefinition, std::vector<SpriteAnimationDefinition>& outAnimations)
    {
        std::ifstream file(path.c_str());
        if (!file.is_open())
            return false;

        try
        {
            nlohmann::json doc;
            file >> doc;

            if (doc.contains("texture")) 
                outDefinition.texturePath = String(doc["texture"].get<std::string>().c_str());

            // Columns
            if (doc.contains("columns"))
            {
                outDefinition.columns = doc["columns"].get<int>();
            }
            else
            {
                outDefinition.columns = 1;
            }

            // Rows
            if (doc.contains("rows"))
            {
                outDefinition.rows = doc["rows"].get<int>();
            }
            else
            {
                outDefinition.rows = 1;
            }

            outDefinition.padding = doc.value("padding", 0);
            outDefinition.margin = doc.value("margin", 0);

            if (doc.contains("animations") && doc["animations"].is_array())
            {
                for (const auto& animJson : doc["animations"])
                {
                    SpriteAnimationDefinition anim;
                    anim.name = String(animJson.value("name", "Animation").c_str());
                    anim.frameRate = animJson.value("frameRate", 12.0f);
                    anim.loop = animJson.value("loop", true);
                    
                    if (animJson.contains("frames") && animJson["frames"].is_array())
                    {
                        for (auto& f : animJson["frames"])
                            anim.frames.push_back(f.get<size_t>());
                    }
                    
                    outAnimations.push_back(anim);
                }
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("SpriteAtlasUtils::ParseAtlasFile: JSON error in " + path + ": " + e.what());
            return false;
        }
        return true;
    }

    std::vector<SpriteFrame> SpriteAtlasUtils::GenerateFrames(Texture& texture, int columns, int rows, int padding, int margin)
    {
        std::vector<SpriteFrame> frames;
        if (!ValidateGrid(columns, rows))
            return frames;

        float texW = texture.GetWidth();
        float texH = texture.GetHeight();

        if (texW <= 0.0f || texH <= 0.0f)
            return frames;

        float cellW = (texW - 2.0f * static_cast<float>(margin) - (static_cast<float>(columns) - 1.0f) * static_cast<float>(padding)) / static_cast<float>(columns);
        float cellH = (texH - 2.0f * static_cast<float>(margin) - (static_cast<float>(rows) - 1.0f) * static_cast<float>(padding)) / static_cast<float>(rows);

        if (cellW <= 0.0f || cellH <= 0.0f)
            return frames;

        frames.reserve(static_cast<size_t>(columns) * rows);

        for (int y = 0; y < rows; ++y)
        {
            for (int x = 0; x < columns; ++x)
            {
                float posX = static_cast<float>(margin) + static_cast<float>(x) * (cellW + static_cast<float>(padding));
                float posY = static_cast<float>(margin) + static_cast<float>(y) * (cellH + static_cast<float>(padding));

                SpriteFrame frame;
                frame.uvRect.x = posX;
                frame.uvRect.y = posY;
                frame.uvRect.width = cellW;
                frame.uvRect.height = cellH;
                frame.texture = &texture; 
                
                frames.push_back(frame);
            }
        }

        return frames;
    }
}