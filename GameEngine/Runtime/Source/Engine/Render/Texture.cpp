#include "Engine/Render/Texture.h"

#include "../../../../ThirdParty/SDL3_image/include/SDL3_image/SDL_image.h"
#include "Core/Logger.h"
#include "SDL3/SDL_render.h"


BixEngine::Render::Texture::~Texture()
{
    if (texture_)
    {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

bool BixEngine::Render::Texture::LoadFromFile(const String& path, SDL_Renderer* renderer)
{
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface)
    {
        LOG_ERROR("Texture::LoadFromFile: failed to load " + path);
        return false;
    }

    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture_)
    {
        LOG_ERROR("Texture::LoadFromFile: failed to create texture from " + path);
        SDL_DestroySurface(surface);
        return false;
    }

    width_ = surface->w;
    height_ = surface->h;

    SDL_DestroySurface(surface);
    format_ = TextureFormat::RGBA8;
    return true;
}

void BixEngine::Render::Texture::Draw(SDL_Renderer* renderer, int x, int y) const
{
    SDL_FRect dest = { static_cast<float>(x), static_cast<float>(y), static_cast<float>(width_), static_cast<float>(height_) };
    SDL_RenderTexture(renderer, texture_, nullptr, &dest);
}
