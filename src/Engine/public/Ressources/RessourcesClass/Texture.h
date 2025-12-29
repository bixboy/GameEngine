#pragma once
#include "Containers/String.h"
#include "Ressources/Core/IResource.h"
#include "SDL3/SDL_render.h"


namespace BixEngine::resources
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

        bool LoadFromFile(const String& path) override;

        
        bool LoadFromFile(const String& path, SDL_Renderer* renderer);

        
        [[nodiscard]] float GetWidth() const noexcept { return width_; }
        [[nodiscard]] float GetHeight() const noexcept { return height_; }

        [[nodiscard]] TextureFormat GetFormat() const noexcept { return format_; }
        [[nodiscard]] void* GetNativeHandle() const noexcept { return texture_; }

        [[nodiscard]] const String& GetPath() const noexcept { return path_; }

        void Draw(SDL_Renderer* renderer, int x, int y) const;

    private:
        SDL_Texture* texture_ = nullptr;
        float width_ = 0;
        float height_ = 0;
        TextureFormat format_ = TextureFormat::Unknown;
        String path_;
    };
}
