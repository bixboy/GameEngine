#include "Ressources/RessourcesClass/Texture.h"
#include <SDL_image.h>
#include "Debug/Logger.h"
#include "Renderer.h"

namespace BixEngine::Resources
{
    Texture::~Texture()
    {
        if (texture_)
        {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }
    }

    Texture::Texture(Texture&& other) noexcept : texture_(other.texture_), width_(other.width_), height_(other.height_),
        format_(other.format_), path_(std::move(other.path_))
    {
        other.texture_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
    }

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            if (texture_)
                SDL_DestroyTexture(texture_);

            texture_ = other.texture_;
            width_ = other.width_;
            height_ = other.height_;
            format_ = other.format_;
            path_ = std::move(other.path_);

            other.texture_ = nullptr;
            other.width_ = 0;
            other.height_ = 0;
        }
        
        return *this;
    }

    bool Texture::LoadFromFile(const String& path)
    {
        auto* renderer = Graphics::Renderer::Get();
        if (!renderer || !renderer->GetSDLRenderer())
        {
            LOG_ERROR("Texture::LoadFromFile: Renderer instance not found for " + path);
            return false;
        }

        return LoadFromFile(path, renderer->GetSDLRenderer());
    }

    bool Texture::LoadFromFile(const String& path, SDL_Renderer* renderer)
    {
        if (!renderer)
            return false;

        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface)
        {
            LOG_ERROR("Texture::LoadFromFile: Failed to load image " + path + ". Error: " + SDL_GetError());
            return false;
        }

        texture_ = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture_)
        {
            LOG_ERROR("Texture::LoadFromFile: Failed to create texture from surface " + path);
            SDL_DestroySurface(surface);
            return false;
        }

        width_ = static_cast<float>(surface->w);
        height_ = static_cast<float>(surface->h);
        path_ = path;

        if (SDL_ISPIXELFORMAT_ALPHA(surface->format))
        {
            format_ = TextureFormat::RGBA8;
        }
        else
        {
            format_ = TextureFormat::RGB8;
        }

        SDL_DestroySurface(surface);
        return true;
    }

    void Texture::Draw(SDL_Renderer* renderer, float x, float y, float w, float h, float rotation, bool flipX, bool flipY) const
    {
        if (!texture_)
            return;

        SDL_FRect destRect = { x, y, w, h };
        
        SDL_FlipMode flip = SDL_FLIP_NONE;
        if (flipX)
            flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
        
        if (flipY)
            flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);

        SDL_RenderTextureRotated(renderer, texture_, nullptr, &destRect, rotation, nullptr, flip);
    }

    void Texture::DrawPart(SDL_Renderer* renderer, const SDL_FRect& srcRect, const SDL_FRect& destRect, float rotation, bool flipX, bool flipY) const
    {
        if (!texture_)
            return;

        SDL_FlipMode flip = SDL_FLIP_NONE;
        
        if (flipX)
            flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
        
        if (flipY)
            flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);

        SDL_RenderTextureRotated(renderer, texture_, &srcRect, &destRect, rotation, nullptr, flip);
    }

    void Texture::SetScaleMode(SDL_ScaleMode mode)
    {
        if (texture_)
            SDL_SetTextureScaleMode(texture_, mode);
    }

    void Texture::SetBlendMode(SDL_BlendMode mode)
    {
        if (texture_)
            SDL_SetTextureBlendMode(texture_, mode);
    }

    void Texture::SetAlpha(Uint8 alpha)
    {
        if (texture_)
            SDL_SetTextureAlphaMod(texture_, alpha);
    }

    void Texture::SetColorMod(Uint8 r, Uint8 g, Uint8 b)
    {
        if (texture_)
            SDL_SetTextureColorMod(texture_, r, g, b);
    }
}