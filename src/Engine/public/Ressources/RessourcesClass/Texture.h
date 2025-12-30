#pragma once
#include "Containers/String.h"
#include "Ressources/Core/IResource.h"
#include "SDL3/SDL_render.h"

struct SDL_Renderer;

namespace BixEngine::Resources
{
    enum class TextureFormat
    {
        Unknown,
        RGBA8,
        RGB8,
        R8,
    };

    class Texture : public IResource
    {
    public:
        Texture() = default;
        ~Texture() override;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        bool LoadFromFile(const String& path) override;
        bool LoadFromFile(const String& path, SDL_Renderer* renderer);

        // --- Rendu Avancé ---
        
        // Dessine la texture complète
        void Draw(SDL_Renderer* renderer, float x, float y, float width, float height, float rotation = 0.0f, bool flipX = false, bool flipY = false) const;
        
        // Dessine une partie de la texture (Pour les Spritesheets / Atlas)
        void DrawPart(SDL_Renderer* renderer, const SDL_FRect& srcRect, const SDL_FRect& destRect, float rotation = 0.0f, bool flipX = false, bool flipY = false) const;

        // --- Configuration ---
        
        // Définit le mode de filtrage (Nearest = Pixel Art, Linear = Lisse)
        void SetScaleMode(SDL_ScaleMode mode);
        
        // Définit le mode de fusion (Alpha Blending, Additive, Modulate...)
        void SetBlendMode(SDL_BlendMode mode);

        // Définit l'opacité (0 = transparent, 255 = opaque)
        void SetAlpha(Uint8 alpha);
        
        // Définit une teinte de couleur (Modulation)
        void SetColorMod(Uint8 r, Uint8 g, Uint8 b);

        // --- Getters ---
        [[nodiscard]] float GetWidth() const noexcept { return width_; }
        [[nodiscard]] float GetHeight() const noexcept { return height_; }
        [[nodiscard]] TextureFormat GetFormat() const noexcept { return format_; }
        [[nodiscard]] SDL_Texture* GetNativeHandle() const noexcept { return texture_; }
        [[nodiscard]] const String& GetPath() const noexcept { return path_; }

    private:
        SDL_Texture* texture_ = nullptr;
        float width_ = 0.0f;
        float height_ = 0.0f;
        TextureFormat format_ = TextureFormat::Unknown;
        String path_;
    };
}