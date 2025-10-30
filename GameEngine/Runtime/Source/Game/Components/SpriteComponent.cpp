#include "Game/Components/SpriteComponent.h"
#include "Game/Actor.h"
#include "Graphics/Renderer.h"
#include "Engine/Render/Texture.h"
#include "SDL3/SDL_render.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr SDL_Color kDefaultSpriteColor{255, 255, 255, 255};
        constexpr float kDefaultSpriteWidth = 32.0f;
        constexpr float kDefaultSpriteHeight = 32.0f;
    }

    SpriteComponent::SpriteComponent(Actor* owner) : Component(owner), color_(kDefaultSpriteColor), width_(kDefaultSpriteWidth), height_(kDefaultSpriteHeight), uvRect_(0.0f, 0.0f, kDefaultSpriteWidth, kDefaultSpriteHeight)
    {
    }

    void SpriteComponent::Render(Graphics::Renderer& renderer) const
    {
        auto pos = owner_->GetPosition();

        SDL_FRect destRect{pos.x, pos.y, width_, height_};

        if (texture_ && texture_->GetNativeHandle())
        {
            SDL_Texture* sdlTexture = static_cast<SDL_Texture*>(texture_->GetNativeHandle());
            SDL_SetTextureColorMod(sdlTexture, color_.r, color_.g, color_.b);
            SDL_SetTextureAlphaMod(sdlTexture, color_.a);

            SDL_FRect srcRect{uvRect_.X, uvRect_.Y, uvRect_.Width, uvRect_.Height};
            SDL_RenderTexture(renderer.GetSDLRenderer(), sdlTexture, &srcRect, &destRect);
        }
        else
        {
            renderer.SetColor(color_.r, color_.g, color_.b, color_.a);
            SDL_RenderFillRect(renderer.GetSDLRenderer(), &destRect);
        }
    }

    void SpriteComponent::SetTexture(Render::Texture* texture) noexcept
    {
        texture_ = texture;

        if (texture_ && !hasCustomUV_)
        {
            uvRect_ = {0.0f, 0.0f, static_cast<float>(texture_->GetWidth()), static_cast<float>(texture_->GetHeight())};
        }
        else if (!texture_)
        {
            hasCustomUV_ = false;
            uvRect_ = {0.0f, 0.0f, width_, height_};
        }
    }

    void SpriteComponent::SetUVRect(const Math::Rect& uvRect) noexcept
    {
        uvRect_ = uvRect;
        hasCustomUV_ = true;
    }
}
