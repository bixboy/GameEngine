#pragma once

#include "Core/Containers/String.h"
#include "SDL3/SDL_render.h"

namespace BixEngine::Ressources
{
    enum class TextureFormat
    {
        Unknown,
        RGBA8,
        RGB8,
        R8,
    };

    class Texture
    {
    public:
        Texture() = default;
        virtual ~Texture();

        // Charge une texture
        bool LoadFromFile(const String& path, SDL_Renderer* renderer);

        // Renvoie la largeur/hauteur en pixels
        [[nodiscard]] int GetWidth() const noexcept { return width_; }
        [[nodiscard]] int GetHeight() const noexcept { return height_; }
        
        [[nodiscard]] TextureFormat GetFormat() const noexcept { return format_; }
        [[nodiscard]] void* GetNativeHandle() const noexcept { return texture_; }

        void Draw(SDL_Renderer* renderer, int x, int y) const;

    private:
        SDL_Texture* texture_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        TextureFormat format_ = TextureFormat::Unknown;
    };
}
